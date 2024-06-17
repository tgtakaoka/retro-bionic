;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z80

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
        ld      A, L
        cpl
        ld      L, A
        ld      A, H
        cpl
        ld      H, A
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
        ld      B, H
        ld      C, L
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

;;; Signed comparison; minuend - subtrahend
;;; @param BC minuend
;;; @param DE subtrahend
;;; @return AF.Z, AF.S
;;; @clobber A
cmp16:
        push    BC
        push    DE
        push    HL
        ld      A, C
        sub     A, E
        ld      L, A
        ld      A, B
        sbc     A, D
        ld      H, A            ; HL=minuend-subtrahend
        or      L
        jr      Z, cmp16_return   ; AF.Z=1
        ld      A, H            ; high(minuend-subtrahend)
        xor     B               ; high(minuend-subtrahend)^minuend)
        ld      L, A
        ld      A, B
        xor     D               ; high(minuend^subtrahend)
        and     L               ; overflow
        xor     H               ; AF.S=overflow^sign
        or      1               ; clear AF.Z
cmp16_return:
        pop     HL
        pop     DE
        pop     BC
        ret

;;; Negate; v
;;; @param DE v
;;; @return DE -v
;;; @clobber A
neg_DE:
        ld      A, D
        cpl
        ld      D, A
        ld      A, E
        cpl
        ld      E, A
        inc     DE
        ret

;;; Unsigned multiplication; result = multiplicand * multiplier
;;; @param BC multiplicand
;;; @param DE multiplier
;;; @return HL result
;;; @clobber BC DE HL A
umul16:
        ld      HL, 0           ; result=0
        jr      umul16_check
umul16_loop:
        or      A, A            ; clear AF.C
        ld      A, D            ; multiplier >>= 1
        rra
        ld      D, A
        ld      A, E
        rra
        ld      E, A
        jr      NC, umul16_next ; if lsb(multiplier) == 0
        add     HL, BC          ; result += multiplicand
umul16_next:
        or      A, A            ; clear AF.C
        ld      A, C            ; multiplicand <<= 1
        rla
        ld      C, A
        ld      A, B
        rla
        ld      B, A
umul16_check:
        ld      A, D
        or      E
        jr      NZ, umul16_loop ; while multiplier != 0
umul16_end:
        ret

;;; Signed multiplication; result = multiplicand * multiplier
;;; @param BC multiplicand
;;; @param DE multiplier
;;; @return HL result
;;; @clobber A BC DE
mul16:
        ld      A, B
        xor     A, D
        push    AF              ; save high(multiplicand^multiplier)
        bit     7, B
        jr      Z, mul16_abs_muliplicand
        ld      A, B
        cpl
        ld      B, A
        ld      A, C
        cpl
        ld      C, A
        inc     BC              ; multiplicand = -multiplicand
mul16_abs_muliplicand:
        bit     7, D
        jr      Z,  mul16_multiply
        ld      A, D
        cpl
        ld      D, A
        ld      A, E
        cpl
        ld      E, A
        inc     DE              ; multiplier = -multiplier
mul16_multiply:
        call    umul16          ; HL = multiplicand * multiplier
        pop     AF              ; A=high(multiplicand^multiplier)
        bit     7, A
        jr      Z, mul16_return
        ld      A, H
        cpl
        ld      H, A
        ld      A, L
        cpl
        ld      L, A
        inc     HL              ; result = -result
mul16_return:
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
        ld      L, 1            ; L=bits
        jr      udiv16_prep
udiv16_prep_loop:
        or      A, A            ; clear AF.C
        ld      A, E            ; divisor <<= 1
        rla
        ld      E, A
        ld      A, D
        rla
        ld      D, A
        inc     L               ; ++bits
udiv16_prep:                    ; while msb(divisor) == 0
        bit     7, D
        jr      Z, udiv16_prep_loop
        ld      A, L
        push    AF              ; push bits
        ld      H, B
        ld      L, C            ; HL=dividend
        xor     A, A
        ld      B, A
        ld      C, A            ; BC=quotient
        jr      udiv16_enter_loop
udiv16_loop:
        push    AF              ; push bits
        or      A, A            ; clear AF.C
        ld      A, D            ; divisor >>= 1
        rra
        ld      D, A
        ld      A, E
        rra
        ld      E, A
        or      A, A            ; clear AF.C
        ld      A, C            ; quotient <<= 1
        rla
        ld      C, A
        ld      A, B
        rla
        ld      B, A
udiv16_enter_loop:
        ld      A, L
        sub     A, E
        ld      L, A
        ld      A, H
        sbc     A, D
        ld      H, A            ; dividend -= divisor
        jr      C, udiv16_readd ; if dividend < 0
        inc     BC              ; quotient |= 1
        jr      udiv16_next
udiv16_readd:
        add     HL, DE          ; dividend += divisor
udiv16_next:
        pop     AF
        dec     A               ; --bits
        jr      NZ, udiv16_loop ; while bits != 0
        push    HL              ; remainder
        push    BC              ; quotient
        pop     HL
        pop     BC
        ret

;;; Signed division; dividend / divisor = quotient ... remainder
;;; @param BC dividend
;;; @param DE divisor
;;; @return BC remainder
;;; @return HL quotient
;;; @clobber A DE
div16:
        ld      A, B
        xor     A, D
        push    AF              ; save high(dividend^divisor)
        bit     7, B
        jr      Z, div16_abs_dividend
        ld      A, B
        cpl
        ld      B, A
        ld      A, C
        cpl
        ld      C, A
        inc     BC              ; dividend = -dividend
div16_abs_dividend:
        bit     7, D
        jr      Z, div16_divide
        ld      A, D
        cpl
        ld      D, A
        ld      A, E
        cpl
        ld      E, A
        inc     DE              ; divisor = -divisor
div16_divide:
        call    udiv16          ; HL=quotient, BC=remainder
        pop     AF              ; A=high(dividend^divisor)
        bit     7, A
        jr      Z, div16_return
        ld      A, H
        cpl
        ld      H, A
        ld      A, L
        cpl
        ld      L, A
        inc     HL              ; quotient=-quotient
div16_return:
        ret
