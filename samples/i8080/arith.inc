;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8085

;;; Print signed 16-bit integer as decimal
;;; @param HL value
;;; @clobber A HL
print_int16:
        mov     A, H
        ora     L
        jz      print_uint16_zero
        mov     A, H
        ora     A
        jp      print_uint16
        mvi     A, '-'
        call    putchar
        mov     A, L
        cma
        mov     L, A
        mov     A, H
        cma
        mov     H, A
        inx     H               ; HL=-value
print_uint16:
        push    B
        push    D
        call    print_uint16_loop
        pop     D
        pop     B
        ret
print_uint16_loop:
        mov     A, H
        ora     L
        rz
        mov     B, H
        mov     C, L
        lxi     D, 10
        call    udiv16          ; BC/DE = HL...BC
        push    B               ; push remainder
        call    print_uint16_loop
        pop     H               ; pop remainder
        mov     A, L
        adi     '0'
        jmp     putchar
print_uint16_zero:
        mvi     A, '0'
        jmp     putchar

;;; Signed comparison; minuend - subtrahend
;;; @param BC minuend
;;; @param DE subtrahend
;;; @return PSW.Z, PSW.S
;;; @clobber A
cmp16:
        push    B
        push    D
        push    H
        ldax    D
        mov     A, C
        sub     E
        mov     L, A
        mov     A, B
        sbb     D
        mov     H, A            ; HL=minuend-subtrahend
        ora     L
        jz      cmp16_return    ; PSW.Z=1
        mov     A, H            ; high(minuend-subtrahend)
        xra     B               ; high(minuend-subtrahend)^minuend
        mov     L, A
        mov     A, B
        xra     D               ; high(minuend^subtrahend)
        ana     L               ; overflow
        xra     H               ; PSW.S=overflow^sign
        ori     1               ; clear PSW.Z
cmp16_return:
        pop     H
        pop     D
        pop     B
        ret

;;; Negate; v
;;; @param DE v
;;; @return DE -v
;;; @clobber A
neg_DE:
        mov     A, D
        cma
        mov     D, A
        mov     A, E
        cma
        mov     E, A
        inx     D
        ret

;;; Unsigned multiplication; result = multiplicand * multiplier
;;; @param BC multiplicand
;;; @param DE multiplier
;;; @return HL result
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

;;; Signed multiplication; result =multiplicand * multiplier
;;; @param BC multiplicand
;;; @param DE multiplier
;;; @return HL result
;;; @clobber A BC DE
mul16:
        mov     A, B
        xra     D
        push    PSW             ; save high(multiplicand^multiplier)
        mov     A, B
        ora     A
        jp      mul16_abs_muliplicand
        cma
        mov     B, A
        mov     A, C
        cma
        mov     C, A
        inx     B               ; multiplicand = -multiplicand
mul16_abs_muliplicand:
        mov     A, D
        ora     A
        jp      mul16_multiply
        cma
        mov     D, A
        mov     A, E
        cma
        mov     E, A
        inx     D               ; multiplier = -multiplier
mul16_multiply:
        call    umul16          ; HL = multiplicand * multiplier
        pop     PSW             ; save high(multiplicand^multiplier)
        ora     A
        jp      mul16_return
        mov     A, H
        cma
        mov     H, A
        mov     A, L
        cma
        mov     L, A
        inx     H               ; result = -result
mul16_return:
        ret

;;; Unsigned division; dividend / divisor = quotient ... remainder
;;; @praram BC dividend
;;; @praram DE divisor
;;; @return BC remainder
;;; @return HL quotient
;;; @clobber A DE
udiv16:
        mov     A, D
        ora     E
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
        inr     L               ; ++bits
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
        push    H               ; remainder
        push    B               ; quotient
        pop     H
        pop     B
        ret

;;; Signed division; dividend / divisor = quotient ... remainder
;;; @param BC dividend
;;; @param DE divisor
;;; @return BC remainder
;;; @return HL quotient
;;; @clobber A DE
div16:
        mov     A, B
        xra     D
        push    PSW             ; save high(dividend^divisor)
        mov     A, B            ; H=high(dividend)
        ora     A
        jp      div16_abs_dividend
        cma
        mov     B, A
        mov     A, C
        cma
        mov     C, A
        inx     B               ; dividend = -dividend
div16_abs_dividend:
        mov     A, D
        ora     A
        jp      div16_divide
        cma
        mov     D, A
        mov     A, E
        cma
        mov     E, A
        inx     D               ; divisor = -divisor
div16_divide:
        call    udiv16          ; HL=quotient, BC=remainder
        pop     PSW             ; A=high(dividend^divisor)
        ora     A
        jp      div16_return
        mov     A, H
        cma
        mov     H, A
        mov     A, L
        cma
        mov     L, A
        inx     H               ; quotient=-quotient
div16_return:
        ret
