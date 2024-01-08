;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8085

;;; Print unsigned 16-bit integer as decimal
;;; @param HL: value
;;; @clobber HL
print_uint16:
        push    PSW
print_uint16_inner:
        push    B
        push    D
        mov     B, H
        mov     C, L
        mov     A, B
        ora     C
        jz      print_uint16_zero
        call    print_uint16_loop
        pop     D
        pop     B
        pop     PSW
        ret
print_uint16_loop:
        mov     A, B
        ora     C
        rz
        lxi     D, 10
        call    udiv16
        push    H               ; push reminder
        call    print_uint16_loop
        pop     H               ; pop reminder
        mov     A, L
        adi     '0'
        jmp     putchar
print_uint16_zero:
        mvi     A, '0'
        call    putchar
        pop     D
        pop     B
        pop     PSW
        ret

;;; Print signed 16-bit integer as decimal
;;; @param HL: value
;;; @clobber HL
print_int16:
        push    PSW
        mov     A, H
        ora     A
        jp      print_uint16_inner
        mvi     A, '-'
        call    putchar
        mov     A, L
        cma
        mov     L, A
        mov     A, H
        cma
        mov     H, A
        inx     H               ; HL=-value
        jmp     print_uint16_inner

;;; Negation; result = -value
;;; @param @BC: result
;;; @param @DE: value
;;; @clobber A
negsi2:
        ldax    D
        cma
        adi     1
        stax    B
        inx     D
        ldax    D
        cma
        aci     0
        inx     B
        stax    B
        dcx     D
        dcx     B
        ret

;;; Signed addition: summand += addend
;;; @param @BC: summand
;;; @param @DE: addend
;;; @clobber A
addsi2:
        push    H
        ldax    D
        mov     L, A
        inx     D
        ldax    D
        mov     H, A
        dcx     D
        ldax    B
        add     L
        stax    B
        inx     B
        ldax    B
        adc     H
        stax    B
        dcx     B
        pop     H
        ret

;;; Singed subtraction: minuend -= subtrahend
;;; @param @BC: minuend
;;; @param @DE: subtrahend
;;; @clobber A
subsi2:
        push    H
        ldax    D
        mov     L, A
        inx     D
        ldax    D
        dcx     D
        mov     H, A
        ldax    B
        sub     L
        stax    B
        inx     B
        ldax    B
        sbb     H
        stax    B
        dcx     B
        pop     H
        ret

;;; Signed comparison: minuend - subtrahend
;;; @param @BC: minuend
;;; @param @DE: subtrahend
;;; @return PSW.Z, PSW.S
;;; @clobber A
cmpsi2:
        push    B
        push    D
        push    H
        ldax    D
        mov     L, A
        inx     D
        ldax    D
        mov     D, A
        mov     E, L            ; DE=@DE
        ldax    B
        mov     L, A
        inx     B
        ldax    B
        mov     B, A
        mov     C, L            ; BC=@BC
        mov     A, C
        sub     E
        mov     L, A
        mov     A, B
        sbb     D
        mov     H, A            ; HL=@BC-@DE
        ora     L
        jz      cmpsi2_return   ; PSW.Z=1
        mov     A, H            ; high(@BC-@DE)
        xra     B               ; high((@BC-@DE)^@BC)
        mov     L, A
        mov     A, B
        xra     D               ; high(@BC^@DE)
        ana     L               ; overflow
        xra     H               ; PSW.S=overflow^sign
        ori     1               ; clear PSW.Z
cmpsi2_return:
        pop     H
        pop     D
        pop     B
        ret

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param BC: multiplicand
;;; @param DE: multiplier
;;; @return HL: result
;;; @clobber BC DE HL A
umul16:
        lxi     H, 0            ; result=0
        jmp     umul16_check
umul16_loop:
        ora     A               ; clear PSW.C
        mov     A, D            ; multiplier >>= 1
        rar
        mov     D, A
        mov     A, E
        rar
        mov     E, A
        jnc     umul16_next     ; if lsb(multiplier) == 0
        dad     B               ; result += multiplicand
umul16_next:
        ora     A               ; clear PSW.C
        mov     A, C            ; multiplicand <<= 1
        ral
        mov     C, A
        mov     A, B
        ral
        mov     B, A
umul16_check:
        mov     A, D
        ora     E
        jnz     umul16_loop     ; while multiplier != 0
umul16_end:
        ret

;;; Signed multiplication: multiplicand *= multiplier
;;; @param @BC: multiplicand
;;; @param @DE: multiplier
;;; @clobber HL A
mulsi2:
        push    B
        push    D
        ldax    B
        mov     L, A
        inx     B
        ldax    B
        mov     B, A
        mov     C, L            ; BC=multiplicand
        mov     H, B            ; H=high(multiplicand)
        ora     A
        jp      mulsi2_abs_muliplicand
        cma
        mov     B, A
        mov     A, C
        cma
        mov     C, A
        inx     B               ; multiplicand = -multiplicand
mulsi2_abs_muliplicand:
        ldax    D
        mov     L, A
        inx     D
        ldax    D
        mov     D, A
        mov     E, L            ; DE=multiplier
        xra     H               ; H=high(multiplicand^multiplier)
        push    PSW             ; save sign
        mov     A, D
        ora     A
        jp      mulsi2_multiply
        cma
        mov     D, A
        mov     A, E
        cma
        mov     E, A
        inx     D               ; multiplier = -multiplier
mulsi2_multiply:
        call    umul16          ; HL = multiplicand * multiplier
        pop     B               ; B=sign
        mov     A, B
        ora     A
        jp      mulsi2_return
        mov     A, H
        cma
        mov     H, A
        mov     A, L
        cma
        mov     L, A
        inx     H               ; result = -result
mulsi2_return:
        pop     D
        pop     B
        mov     A, L
        stax    B
        inx     B
        mov     A, H
        stax    B
        dcx     B               ; @BC=result
        ret

;;; Unsigned division: dividend / divisor = quotient ... reminder
;;; @praram BC: dividend
;;; @praram DE: divisor
;;; @return HL: reminder
;;; @return BC: quotient
;;; @clobber BC DE HL A
udiv16:
        mov     A, E
        ora     D
        rz                      ; divide by zero
        mvi     L, 1            ; L=bits
        jmp     udiv16_prep
udiv16_prep_loop:
        ora     A               ; clear PSW.C
        mov     A, E            ; divisor <<= 1
        ral
        mov     E, A
        mov     A, D
        ral
        mov     D, A
        inx     H               ; ++bits
udiv16_prep:                    ; while msb(divisor) == 0
        mov     A, D
        ora     A
        jp      udiv16_prep_loop
        mov     A, L
        push    PSW             ; push bits
        mov     H, B
        mov     L, C            ; HL=dividend
        xra     A
        mov     B, A
        mov     C, A            ; BC=quotient
        jmp     udiv16_enter_loop
udiv16_loop:
        push    PSW             ; push bits
        ora     A               ; clear PSW.C
        mov     A, D            ; divisor >>= 1
        rar
        mov     D, A
        mov     A, E
        rar
        mov     E, A
        ora     A               ; clear PSW.C
        mov     A, C            ; quotient <<= 1
        ral
        mov     C, A
        mov     A, B
        ral
        mov     B, A
udiv16_enter_loop:
        mov     A, L
        sub     E
        mov     L, A
        mov     A, H
        sbb     D
        mov     H, A            ; dividend -= divisor
        jc      udiv16_readd    ; if dividend < 0
        inx     B               ; quotient |= 1
        jmp     udiv16_next    ;
udiv16_readd:
        dad     D               ; dividend += divisor
udiv16_next:
        pop     PSW
        dcr     A               ; --bits
        jnz     udiv16_loop     ; while bits != 0
        ret

;;; Signed division: dividend *= divisor
;;; @param @BC: dividend
;;; @param @DE: divisor
;;; @clobber HL A
divsi2:
        push    B
        push    D
        ldax    B
        mov     L, A
        inx     B
        ldax    B
        mov     B, A
        mov     C, L            ; BC=dividend
        mov     H, B            ; H=high(dividend)
        ora     A
        jp      divsi2_abs_dividend
        cma
        mov     B, A
        mov     A, C
        cma
        mov     C, A
        inx     B               ; dividend = -dividend
divsi2_abs_dividend:
        ldax    D
        mov     L, A
        inx     D
        ldax    D
        mov     D, A
        mov     E, L            ; DE=divisor
        xra     H               ; H=high(dividend^divisor)
        push    PSW             ; save sign
        mov     A, D
        ora     A
        jp      divsi2_divide
        cma
        mov     D, A
        mov     A, E
        cma
        mov     E, A
        inx     D               ; divisor = -divisor
divsi2_divide:
        call    udiv16          ; BC = dividend / divisor
        mov     H, B
        mov     L, C            ; HL=quotient
        pop     B               ; B=sign
        mov     A, B
        ora     A
        jp      divsi2_return
        mov     A, H
        cma
        mov     H, A
        mov     A, L
        cma
        mov     L, A
        inx     H               ; quotient=-quotient
divsi2_return:
        pop     D
        pop     B
        mov     A, L
        stax    B
        inx     B
        mov     A, H
        stax    B
        dcx     B               ; @BC=result
        ret
