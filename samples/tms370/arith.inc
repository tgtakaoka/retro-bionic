;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print signed 16-bit integer as decimal
;;; @param R4:R5 value
;;; @clobber A R4:R5
print_int16:
        btjz    #080H, R4, print_uint16
        mov     #'-', A
        call    putchar
        inv     R4
        inv     R5
        inc     R5
        adc     #0, R4    ; negate R4:R5
;;; Print unsigned 16-bit integer as decimal
;;; @param R4:R5 value
;;; @clobber A B R4:R5
print_uint16:
        mov     R4, A
        or      R5, A
        jz      print_uint16_zero
print_uint16_loop:
        mov     R4, A
        or      R5, A
        jz      print_uint16_end
        push    R6
        mov     #10, R7
        call    udiv16_8        ; R4:R5/R6 => R4/R5...B
        pop     R6
        push    B               ; push reminder
        call    print_uint16_loop
        pop     A
print_uint16_zero:
        add     #'0', A
        br      putchar
print_uint16_end:
        rts

;;; Divide unsigned 16bit by 8bit
;;; @param R4:R5 dividend
;;; @param R7 divisor
;;; @return R4:R5 quotient
;;; @return B reminder
;;; @clobber A
udiv16_8:
        mov     R4, B
        clr     A               ; A:B=0:R4
        div     R7, A           ; A=R4/R7, B=R4%R7
        mov     A, R4           ; R4=high(quotient)
        mov     B, A
        mov     R5, B           ; A:B=R4%R6:R5
        div     R7, A           ; A=low(quotient), B=reminder
        mov     A, R5           ; R5=low(quotient)
        rts

;;; Addition: R4:R5 += R6:R7
add16:
        add     R7, R5
        adc     R6, R4
        rts

;;; Subtraction: R4:R5 -= R6:R7
sub16:
        sub     R7, R5
        sbb     R6, R4
        rts

;;; Signed compare A=sign(minuend-subtrahend)
;;; @param R4:R5 minuend
;;; @param R6:R7 subtrahend
;;; @return A=0  JEQ (minuend == subtrahend)
;;;         A=1  JGT (minuend > subtrahend)
;;;         A=-1 JLT (minuend < subtrahend)
;;; @clobber A
cmp16:
        push    R3
        push    R2
        movw    R5, R3          ; R2:R3=minuend
        sub     R7, R3
        sbb     R6, R2          ; R2:R3=minuend-subtrahend
        mov     R2, A
        or      R3, A
        jeq     cmp16_end       ; return with A=0
        mov     R2, A           ; A=hi(minuend-subtrahend)
        xor     R4, A           ; A=hi((minuend-subtrahend)^minuend)
        mov     A, R3           ; R3=hi((minuend-subtrahend)^minuend)
        mov     R4, A           ; A=hi(minuend)
        xor     R6, A           ; A=hi(minuend^subtrahend)
        and     R3, A           ; A=overflow flag
        xor     R2, A           ; A=V^N
        jn      cmp16_lt        ; branch if minuend < subtrahend
cmp16_gt:
        mov     #1, A
        jmp     cmp16_end
cmp16_lt:
        mov     #-1, A
cmp16_end:
        pop     R2
        pop     R3
        tst     A
        rts

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param R4:R5 multiplicand
;;; @param R6:R7 multiplier
;;; @return R4:R5 result
;;; @clobber A B
umul16:
        push    R3
        push    R2
        clr     R2
        clr     R3
        mpy     R5, R7          ; A:B=R5*R7
        movw    B, R3           ; R2:R3=R5*R7
        mpy     R5, R6          ; A:B=R5*R6
        add     B, R2           ; R2+=low(R5*R6)
        mpy     R4, R7          ; A:B=R4*R7
        add     B, R2           ; R2+=low(R4*R7)
        movw    R3, R5          ; R4:R5=result
        pop     R2
        pop     R3
        rts

;;; Multiply: result = multiplicand * multiplier
;;; @param R4:R5 multiplicand
;;; @param R6:R7 multiplier
;;; @return R4:R5 result
;;; @clobber A R6:R7
mul16:
        mov     R4, A
        xor     R6, A
        push    A               ; save hi(multiplicand^multiplier)
        btjz    #080H, R6, mul16_multiplicand
        inv     R6
        inv     R7
        inc     R7
        adc     #0, R6          ; negate multiplier
mul16_multiplicand:
        btjz    #080H, R4, mul16_multiply
        inv     R4
        inv     R5
        inc     R5
        adc     #0, R4          ; negate multiplicand
mul16_multiply:
        call    umul16          ; R4:R5=result
        pop     A               ; A=(multiplicand^multiplier)
        jpz     mul16_end
        inv     R4
        inv     R5
        inc     R5
        adc     #0, R4          ; negate result
mul16_end:
        rts

;;; Unsigned division: dividend / divisor = quotient ... reminder
;;; @praram R4:R5 dividend
;;; @praram R6:R7 divisor
;;; @return R4:R5 quotient
;;; @return R6:R7 reminder
;;; @clobber A B
udiv16:
        mov     R6, A
        jnz     udiv16_16
        call    udiv16_8        ; R4:R5/R7=R4:R5...B
        mov     B, R7
        rts
udiv16_16:      
        or      R7, A
        jz      udiv16_end      branch if divisor==0
        push    R8
        mov     #16, R8
        clr     A
        clr     B               ; initialize quotient
udiv16_loop:
        rlc     R5
        rlc     R4              ; dividend <<= 1
        rlc     B
        rlc     A               ;
        jnc     udiv16_skip
        sub     R7, B
        sbb     R6, A
        setc
        jmp     udiv16_next
udiv16_skip:
        cmp     R6, A
        jnc     udiv16_next
        jne     udiv16_sub
        cmp     R7, B
        jnc     udiv16_next
udiv16_sub:
        sub     R7, B
        sbb     R6, A
udiv16_next:
        djnz    R8, udiv16_loop
        rlc     R5
        rlc     R4
        movw    R1, R7          ; save reminder
        pop     R8
udiv16_end:
        rts

;;; Division: dividend / divisor = quotient ... reminder
;;; @param R4:R5 dividend
;;; @param R6:R7 divisor
;;; @return R4:R5 quotient
;;; @return R6:R7 reminder
;;; @clobber A
div16:
        mov     R4, A
        xor     R6, A
        push    A               ; save hi(dividend^divisor)
        btjz    #080H, R6, div16_dividend
        inv     R6
        inv     R7
        inc     R7
        adc     #0, R6          ; negate divisor
div16_dividend:
        btjz    #080H, R4, div16_divide
        inv     R4
        inv     R5
        inc     R5
        adc     #0, R4          ; negate divisor
div16_divide:
        call    udiv16          ; R4:R5=quotient
        pop     A               ; A=(dividend^divisor)
        jpz     div16_end
        inv     R4
        inv     R5
        inc     R5
        adc     #0, R4          ; negate quotient
div16_end:
        rts
