;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z80

;;; Print unsigned 16-bit integer as decimal
;;; @param HL: value
;;; @clobber HL
print_uint16:
        push    AF
print_uint16_inner:
        push    BC
        push    DE
        ld      B, H
        ld      C, L
        ld      A, B
        or      C
        jr      Z, print_uint16_zero
        call    print_uint16_loop
        pop     DE
        pop     BC
        pop     AF
        ret
print_uint16_loop:
        ld      A, B
        or      C
        ret     Z
        ld      DE, 10
        call    udiv16
        push    HL              ; push reminder
        call    print_uint16_loop
        pop     HL              ; pop reminder
        ld      A, L
        add     A, '0'
        jp      putchar
print_uint16_zero:
        ld      A, '0'
        call    putchar
        pop     DE
        pop     BC
        pop     AF
        ret

;;; Print signed 16-bit integer as decimal
;;; @param HL: value
;;; @clobber HL
print_int16:
        push    AF
        bit     7, H
        jr      Z, print_uint16_inner
        ld      A, '-'
        call    putchar
        ld      A, L
        cpl
        ld      L, A
        ld      A, H
        cpl
        ld      H, A
        inc     HL              ; HL=-value
        jr      print_uint16_inner

;;; Negation; result = -value
;;; @param @DE: result
;;; @param @BC: value
;;; @clobber A
negsi2:
        ld      A, (DE)
        cpl
        add     A, 1
        ld      (BC), A
        inc     DE
        ld      A, (DE)
        cpl
        adc     A, 0
        inc     BC
        ld      (BC), A
        dec     DE
        dec     BC
        ret

;;; Signed addition: summand += addend
;;; @param @BC: summand
;;; @param @DE: addend
;;; @clobber A
addsi2:
        push    HL
        ld      A, (DE)
        ld      L, A
        inc     DE
        ld      A, (DE)
        ld      H, A
        dec     DE
        ld      A, (BC)
        add     A, L
        ld      (BC), A
        inc     BC
        ld      A, (BC)
        adc     A, H
        ld      (BC), A
        dec     BC
        pop     HL
        ret

;;; Singed subtraction: minuend -= subtrahend
;;; @param @BC: minuend
;;; @param @DE: subtrahend
;;; @clobber A
subsi2:
        push    HL
        ld      A, (DE)
        ld      L, A
        inc     DE
        ld      A, (DE)
        ld      H, A
        dec     DE
        ld      A, (BC)
        sub     A, L
        ld      (BC), A
        inc     BC
        ld      A, (BC)
        sbc     A, H
        ld      (BC), A
        dec     BC
        pop     HL
        ret

;;; Signed comparison: minuend - subtrahend
;;; @param @BC: minuend
;;; @param @DE: subtrahend
;;; @return AF.Z, AF.S
;;; @clobber A
cmpsi2:
        push    BC
        push    DE
        push    HL
        ld      A, (DE)
        ld      L, A
        inc     DE
        ld      A, (DE)
        ld      D, A
        ld      E, L            ; DE=@DE
        ld      A, (BC)
        ld      L, A
        inc     BC
        ld      A, (BC)
        ld      B, A
        ld      C, L            ; BC=@BC
        ld      A, C
        sub     A, E
        ld      L, A
        ld      A, B
        sbc     A, D
        ld      H, A            ; HL=@BC-@DE
        or      L
        jr      Z, cmpsi2_return   ; AF.Z=1
        ld      A, H            ; high(@BC-@DE)
        xor     B               ; high((@BC-@DE)^@BC)
        ld      L, A
        ld      A, B
        xor     D               ; high(@BC^@DE)
        and     L               ; overflow
        xor     H               ; AF.S=overflow^sign
        or      1               ; clear AF.Z
cmpsi2_return:
        pop     HL
        pop     DE
        pop     BC
        ret

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param BC: multiplicand
;;; @param DE: multiplier
;;; @return HL: result
;;; @clobber BC DE HL A
umul16:
        ld      HL, 0           ; result=0
        jr      umul16_check
umul16_loop:
        or      A               ; clear AF.C
        ld      A, D            ; multiplier >>= 1
        rra
        ld      D, A
        ld      A, E
        rra
        ld      E, A
        jr      NC, umul16_next ; if lsb(multiplier) == 0
        add     HL, BC          ; result += multiplicand
umul16_next:
        or      A               ; clear AF.C
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

;;; Signed multiplication: multiplicand *= multiplier
;;; @param @BC: multiplicand
;;; @param @DE: multiplier
;;; @clobber HL A
mulsi2:
        push    BC
        push    DE
        ld      A, (BC)
        ld      L, A
        inc     BC
        ld      A, (BC)
        ld      B, A
        ld      C, L            ; BC=multiplicand
        ld      H, B            ; H=high(multiplicand)
        bit     7, A
        jr      Z, mulsi2_abs_muliplicand
        cpl
        ld      B, A
        ld      A, C
        cpl
        ld      C, A
        inc     BC              ; multiplicand = -multiplicand
mulsi2_abs_muliplicand:
        ld      A, (DE)
        ld      L, A
        inc     DE
        ld      A, (DE)
        ld      D, A
        ld      E, L            ; DE=multiplier
        xor     H               ; H=high(multiplicand^multiplier)
        push    AF              ; save sign
        ld      A, D
        bit     7, A
        jr      Z,  mulsi2_multiply
        cpl
        ld      D, A
        ld      A, E
        cpl
        ld      E, A
        inc     DE              ; multiplier = -multiplier
mulsi2_multiply:
        call    umul16          ; HL = multiplicand * multiplier
        pop     BC              ; B=sign
        bit     7, B
        jr      Z, mulsi2_return
        ld      A, H
        cpl
        ld      H, A
        ld      A, L
        cpl
        ld      L, A
        inc     HL              ; result = -result
mulsi2_return:
        pop     DE
        pop     BC
        ld      A, L
        ld      (BC), A
        inc     BC
        ld      A, H
        ld      (BC), A
        dec     BC              ; @BC=result
        ret

;;; Unsigned division: dividend / divisor = quotient ... reminder
;;; @praram BC: dividend
;;; @praram DE: divisor
;;; @return HL: reminder
;;; @return BC: quotient
;;; @clobber BC DE HL A
udiv16:
        ld      A, E
        or      D
        ret     Z               ; divide by zero
        ld      L, 1            ; L=bits
        jr      udiv16_prep
udiv16_prep_loop:
        or      A               ; clear AF.C
        ld      A, E            ; divisor <<= 1
        rla
        ld      E, A
        ld      A, D
        rla
        ld      D, A
        inc     HL              ; ++bits
udiv16_prep:                    ; while msb(divisor) == 0
        bit     7, D
        jr      Z, udiv16_prep_loop
        ld      A, L
        push    AF              ; push bits
        ld      H, B
        ld      L, C            ; HL=dividend
        xor     A
        ld      B, A
        ld      C, A            ; BC=quotient
        jr      udiv16_enter_loop
udiv16_loop:
        push    AF              ; push bits
        or      A               ; clear AF.C
        ld      A, D            ; divisor >>= 1
        rra
        ld      D, A
        ld      A, E
        rra
        ld      E, A
        or      A               ; clear AF.C
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
        ret

;;; Signed division: dividend *= divisor
;;; @param @BC: dividend
;;; @param @DE: divisor
;;; @clobber HL A
divsi2:
        push    BC
        push    DE
        ld      A, (BC)
        ld      L, A
        inc     BC
        ld      A, (BC)
        ld      B, A
        ld      C, L            ; BC=dividend
        ld      H, B            ; H=high(dividend)
        bit     7, A
        jr      Z, divsi2_abs_dividend
        cpl
        ld      B, A
        ld      A, C
        cpl
        ld      C, A
        inc     BC              ; dividend = -dividend
divsi2_abs_dividend:
        ld      A, (DE)
        ld      L, A
        inc     DE
        ld      A, (DE)
        ld      D, A
        ld      E, L            ; DE=divisor
        xor     H               ; H=high(dividend^divisor)
        push    AF              ; save sign
        ld      A, D
        bit     7, A
        jr      Z, divsi2_divide
        cpl
        ld      D, A
        ld      A, E
        cpl
        ld      E, A
        inc     DE              ; divisor = -divisor
divsi2_divide:
        call    udiv16          ; BC = dividend / divisor
        ld      H, B
        ld      L, C            ; HL=quotient
        pop     BC              ; B=sign
        bit     7, B
        jr      Z, divsi2_return
        ld      A, H
        cpl
        ld      H, A
        ld      A, L
        cpl
        ld      L, A
        inc     HL              ; quotient=-quotient
divsi2_return:
        pop     DE
        pop     BC
        ld      A, L
        ld      (BC), A
        inc     BC
        ld      A, H
        ld      (BC), A
        dec     BC               ; @BC=result
        ret
