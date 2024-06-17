;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tlcs90

;;; Print signed 16-bit integer as decimal
;;; @param HL value
;;; @clobber A HL
print_int16:
        ld      A, H
        or      A, L
        jr      Z, print_uint16_zero
        bit     7, H
        jr      Z, print_uint16
        ld      A, '-'
        call    putchar
        xor     HL, 0FFFFH
        inc     HL              ; HL=-value
print_uint16:
        push    BC
        push    DE
        call    print_uint16_loop
        pop     DE
        pop     BC
        ret
print_uint16_loop:
        ld      A, H
        or      A, L
        ret     Z
        ld      BC, HL
        ld      DE, 10
        call    udiv16          ; BC/DE = HL...BC
        push    BC              ; push remainder
        call    print_uint16_loop
        pop     HL              ; pop remainder
        ld      A, L
        add     A, '0'
        jp      putchar
print_uint16_zero:
        ld      A, '0'
        jp      putchar

;;; Unsigned multiplication; product = multiplicand * multiplier
;;; @param BC multiplicand
;;; @param DE multiplier
;;; @return HL product
;;; @clobber BC DE HL A
umul16:
        ld      L, B            ; L=high(multiplicand)
        mul     HL, E           ; HL=high(multiplicand)*low(multiplier)
        push    HL              ; push high(multiplicand)*low(multiplier)
        ld      L, C            ; L=low(multiplicand)
        mul     HL, D           ; HL=low(multiplicand)*high(multiplier)
        ld      B, L            ; B=low(low(multiplicand)*high(multiplier))
        ld      L, C
        mul     HL, E           ; HL=low(multiplicand)*low(multiplier)
        pop     DE              ; DE=high(multiplicand)*low(multiplier)
        ld      A, H
        add     A, E
        add     A, B
        ld      H, A
        ret


;;; Signed multiplication; result = multiplicand * multiplier
;;; @param BC multiplicand
;;; @param DE multiplier
;;; @treturn HL result
;;; @clobber A
mul16:
        ld      A, B
        xor     A, D
        push    AF              ; push high(multiplicand^multiplier)
        bit     7, B
        jr      Z, mul16_abs_muliplicand
        ld      HL, BC
        xor     HL, 0FFFFH
        inc     HL
        ld      BC, HL          ; multiplicand = -multiplicand
mul16_abs_muliplicand:
        bit     7, D
        jr      Z, mul16_multiply
        ld      HL, DE
        xor     HL, 0FFFFH
        inc     HL
        ld      DE, HL          ; multiplier = -multiplier
mul16_multiply:
        call    umul16          ; HL = multiplicand * multiplier
        pop     AF              ; A=sign
        bit     7, A
        ret     Z
        xor     HL, 0FFFFH
        inc     HL              ; result = -result
        ret

;;; Unsigned division; dividend / divisor = quotient ... remainder
;;; @praram BC dividend
;;; @praram DE divisor
;;; @return BC remainder
;;; @return HL quotient
;;; @clobber A DE
udiv16:
        ld      A, D
        or      A, E
        ret     Z               ; divide by zero
        inc     D
        dec     D
        jr      nz, udiv16_16   ; branch if high(divisor)!=0
        ld      HL, BC
        div     HL, E           ; calculate BC/E
        jr      ov, udiv16_8    ; quotient is overflowed
        ld      C, H            ; H=remainder, L=quotient
        ld      H, 0
        ld      B, 0
        ret
udiv16_8:
        ld      A, E
        push    BC              ; save dividend
        ld      L, B            ; high(divided)
        ld      H, 0
        div     HL, A
        ld      B, L            ; high(quotient)=high(dividend)/divisor
        ld      D, H            ; high(remainder)=high(dividend)%divisor
        pop     HL              ; restore dividend
        ld      H, 0            ; low(dividend)
        div     HL, A
        ld      C, L            ; low(quotient)=low(dividend)/divisor
        ld      E, H            ; low(remainder)=low(dividend)%divisor
        ld      HL, DE
        div     HL, A           ; H=remainder, L=quotient
        push    HL              ; save remainder
        ld      H, 0
        add     HL, BC          ; adjust quotient
        pop     BC              ; restore remainder
        ld      C, B
        ld      C, 0
        ret
udiv16_16:
        ld      A, 1            ; A=bits
        jr      udiv16_prep
udiv16_prep_loop:
        sll     E
        rl      D               ; divisor <<= 1
        inc     A               ; ++bits
udiv16_prep:                    ; while msb(divisor) == 0
        bit     7, D
        jr      Z, udiv16_prep_loop
        ld      HL, BC          ; HL=dividend
        ld      BC, 0           ; BC=quotient
        jr      udiv16_enter_loop
udiv16_loop:
        srl     D
        rr      E               ; divisor >>= 1
        sll     C
        rl      B               ; quotient <<= 1
udiv16_enter_loop:
        sub     HL, DE          ; dividend -= divisor
        jr      ULT, udiv16_readd ; if dividend < 0
        inc     C                 ; quotient |= 1
        dec     A
        jr      NZ, udiv16_loop ; while bits != 0
        ret
udiv16_readd:
        add     HL, DE          ; dividend += divisor
        dec     A               ; --bits
        jr      NZ, udiv16_loop ; while bits != 0
        ld      DE, HL
        ld      HL, BC
        ld      BC, DE
        ret

;;; Signed division; dividend / divisor = quotient ... remainder
;;; @param BC dividend
;;; @param DE divisor
;;; @return HL quotient
;;; @return BC remainder
;;; @clobber DE A
div16:
        ld      A, B
        xor     A, D
        push    AF              ; push high(dividend^divisor)
        bit     7, B
        jr      Z, div16_abs_dividend
        ld      HL, BC
        xor     HL, 0FFFFH
        inc     HL
        ld      BC, HL          ; dividend = -dividend
div16_abs_dividend:
        bit     7, D
        jr      Z, div16_divide
        ld      HL, DE
        xor     HL, 0FFFFH
        inc     HL
        ld      DE, HL          ; divisor = -divisor
div16_divide:
        call    udiv16          ; BC = dividend / divisor
        pop     AF              ; A=sign
        bit     7, A
        ret     Z
        xor     HL, 0FFFFH
        inc     HL              ; quotient=-quotient
        ret
