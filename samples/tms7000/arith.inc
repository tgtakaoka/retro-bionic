;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tms7000

        *** Print signed 16-bit integer as decimal
        *** @param R4:R5 value
        *** @clobber A R4:R5 R6:R7
print_int16:
        btjz    %>80, R4, print_uint16
        mov     %'-', A
        call    @putchar
        inv     R4
        inv     R5
        inc     R5
        adc     %0, R4          ; negate R4:R5
        *** Print unsigned 16-bit integer as decimal
        *** @param R4:R5 value
        *** @clobber A R4:R5 R6:R7
print_uint16:
        mov     R4, A
        or      R5, A
        jz      print_uint16_zero
print_uint16_loop:
        mov     R4, A
        or      R5, A
        jz      print_uint16_end ; branch if value == 0
        movd    %10, R7          ; R6:R7=10
        call    @udiv16          ; R4:R5=value/10
        push    R7               ; push reminder
        call    @print_uint16_loop
        pop     A
print_uint16_zero:
        or      %'0', A
        br      @putchar
print_uint16_end:
        rets

        *** Addition: R4:R5 += R6:R7
add16:
        add     R7, R5
        adc     R6, R4
        rets

        *** Subtraction: R4:R5 -= R6:R7
sub16:
        sub     R7, R5
        sbb     R6, R4
        rets

        *** Signed compare A=sign(minuend-subtrahend)
        *** @param R4:R5 minuend
        *** @param R6:R7 subtrahend
        *** @return A=0  JEQ (minuend == subtrahend)
        ***         A=1  JGT (minuend > subtrahend)
        ***         A=-1 JLT (minuend < subtrahend)
        *** @clobber A
cmp16:
        push    R3
        push    R2
        movd    R5, R3          ; R2:R3=minuend
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
        mov     %1, A
        jmp     cmp16_end
cmp16_lt:
        mov     %-1, A
cmp16_end:
        pop     R2
        pop     R3
        tsta
        rets

        *** Unsigned multiplication: result = multiplicand * multiplier
        *** @param R4:R5 multiplicand
        *** @param R6:R7 multiplier
        *** @return R4:R5 result
        *** @clobber A R4:R5 R6:R7
umul16:
        push    R3
        push    R2
        movd    R5, R3          ; R2:R3=multiplicand
        movd    %0, R5          ; R4:R5=result=0
        jmp     umul16_check
umul16_loop:
        clrc
        rrc     R6
        rrc     R7              ; multiplier >>= 1
        jnc     umul16_next     ; if lsb(multiplier) == 0
        add     R3, R5
        adc     R2, R4          ; result += multiplicand
umul16_next:
        clrc
        rlc     R3
        rlc     R2              ; multiplicand <<= 1
umul16_check:
        mov     R6, A
        or      R7, A
        jnz     umul16_loop     ; while multiplier != 0
umul16_end:
        pop     R2
        pop     R3
        rets

        *** Multiply: result = multiplicand * multiplier
        *** @param R4:R5 multiplicand
        *** @param R6:R7 multiplier
        *** @return R4:R5 result
        *** @clobber A R6:R7
mul16:
        mov     R4, A
        xor     R6, A
        push    A               ; save hi(multiplicand^multiplier)
        btjz    %>80, R6, mul16_multiplicand
        inv     R6
        inv     R7
        inc     R7
        adc     %0, R6          ; negate multiplier
mul16_multiplicand:
        btjz    %>80, R4, mul16_multiply
        inv     R4
        inv     R5
        inc     R5
        adc     %0, R4          ; negate multiplicand
mul16_multiply:
        call    @umul16         ; R4:R5=result
        pop     A               ; A=(multiplicand^multiplier)
        jpz     mul16_end
        inv     R4
        inv     R5
        inc     R5
        adc     %0, R4          ; negate result
mul16_end:
        rets

        *** Unsigned division: dividend / divisor = quotient ... reminder
        *** @praram R4:R5 dividend
        *** @praram R6:R7 divisor
        *** @return R4:R5 quotient
        *** @return R6:R7 reminder
        *** @clobber A
udiv16:
        mov     R6, A
        or      R7, A
        jz      udiv16_end      ; branch if divisor==0
        push    R8
        mov     %1, R8          ; R8=bits
        jmp     udiv16_prep
udiv16_prep_loop:
        clrc
        rlc     R7
        rlc     R6              ; divisor <<= 1
        inc     R8              ; ++bits
udiv16_prep:
        btjz    %>80, R6, udiv16_prep_loop ; while msb(divisor) == 0
        push    R3
        push    R2
        movd    %0, R3          ; R2:R3=quotient=0
        jmp     udiv16_enter_loop
udiv16_loop:
        clrc
        rrc     R6
        rrc     R7              ; divisor >>= 1
        clrc
        rlc     R3
        rlc     R2              ; quotient <<= 1
udiv16_enter_loop:
        sub     R7, R5
        sbb     R6, R4          ; dividend -= divisor
        jl      udiv16_readd
        inc     R3              ; quotient |= 1
        jmp     udiv16_next
udiv16_readd:
        add     R7, R5
        adc     R6, R4          ; dividend += divisor
udiv16_next:
        djnz    R8, udiv16_loop ; while bits-- != 0
        movd    R5, R7          ; R6:R7=reminder
        movd    R3, R5          ; R4:R5=quotient
        pop     R2
        pop     R3
        pop     R8
udiv16_end:
        rets

        *** Division: dividend / divisor = quotient ... reminder
        *** @param R4:R5 dividend
        *** @param R6:R7 divisor
        *** @return R4:R5 quotient
        *** @return R6:R7 reminder
        *** @clobber A
div16:
        mov     R4, A
        xor     R6, A
        push    A               ; save hi(dividend^divisor)
        btjz    %>80, R6, div16_dividend
        inv     R6
        inv     R7
        inc     R7
        adc     %0, R6          ; negate divisor
div16_dividend:
        btjz    %>80, R4, div16_divide
        inv     R4
        inv     R5
        inc     R5
        adc     %0, R4          ; negate dividend
div16_divide:
        call    @udiv16         ; R4:R5=quotient
        pop     A               ; A=(dividend^divisor)
        jpz     div16_end
        inv     R4
        inv     R5
        inc     R5
        adc     %0, R4          ; negate quotient
div16_end:
        rets
