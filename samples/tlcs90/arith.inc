;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tlcs90


;;; Print signed 16-bit integer as decimal
;;; @param HL: value
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
        call    udiv16          ; BC / DE=BC...HL
        push    BC              ; push reminder
        call    print_uint16_loop
        pop     HL              ; pop reminder
        ld      A, L
        add     A, '0'
        jp      putchar
print_uint16_zero:
        ld      A, '0'
        jp      putchar

;;; Unsigned multiplication: product = multiplicand * multiplier
;;; @param BC: multiplicand
;;; @param DE: multiplier
;;; @return HL: product
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


;;; Signed multiplication: result = multiplicand * multiplier
;;; @param BC: multiplicand
;;; @param DE: multiplier
;;; @treturn HL: result
;;; @clobber A
mulsi2:
        ld      A, B
        xor     A, D
        push    AF              ; push high(multiplicand^multiplier)
        ld      HL, BC
        bit     7, H
        jr      Z, mulsi2_abs_muliplicand
        xor     HL, 0FFFFH
        inc     HL              ; multiplicand = -multiplicand
mulsi2_abs_muliplicand:
        ld      BC, HL          ; BC=abs(multiplicand)
        ld      HL, DE
        bit     7, H
        jr      Z, mulsi2_multiply
        xor     HL, 0FFFFH
        inc     HL              ; multiplier = -multiplier
mulsi2_multiply:
        ld      DE, HL          ; DE=abs(multiplyer)
        call    umul16          ; HL = multiplicand * multiplier
        pop     AF              ; A=sign
        bit     7, A
        ret     Z
        xor     HL, 0FFFFH
        inc     HL              ; result = -result
        ret

;;; Unsigned division: dividend / divisor = quotient ... reminder
;;; @praram BC: dividend
;;; @praram DE: divisor
;;; @return BC: reminder
;;; @return HL: quotient
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
        ld      C, H            ; H=reminder, L=quotient
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
        ld      D, H            ; high(reminder)=high(dividend)%divisor
        pop     HL              ; restore dividend
        ld      H, 0            ; low(dividend)
        div     HL, A
        ld      C, L            ; low(quotient)=low(dividend)/divisor
        ld      E, H            ; low(reminder)=low(dividend)%divisor
        ld      HL, DE
        div     HL, A           ; H=reminder, L=quotient
        push    HL              ; save reminder
        ld      H, 0
        add     HL, BC          ; adjust quotient
        pop     BC              ; restore reminder
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

;;; Signed division: dividend / divisor = quotient ... reminder
;;; @param BC: dividend
;;; @param DE: divisor
;;; @return HL: quotient
;;; @return BC: reminder
;;; @clobber DE A
divsi2:
        ld      A, B
        xor     A, D
        push    AF              ; push high(dividend^divisor)
        ld      HL, BC
        bit     7, H
        jr      Z, divsi2_abs_dividend
        xor     HL, 0FFFFH
        inc     HL              ; dividend = -dividend
divsi2_abs_dividend:
        ld      BC, HL          ; BC=abs(dividend)
        ld      HL, DE
        bit     7, H
        jr      Z, divsi2_divide
        xor     HL, 0FFFFH
        inc     HL              ; divisor = -divisor
divsi2_divide:
        ld      DE, HL          ; DE=abs(dvisor)
        call    udiv16          ; BC = dividend / divisor
        pop     AF              ; A=sign
        bit     7, A
        ret     Z
        xor     HL, 0FFFFH
        inc     HL              ; quotient=-quotient
        ret
