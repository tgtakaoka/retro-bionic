;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tms7000

        *** Print signed 16-bit integer as decimal
        *** @param R4:R5 value
        *** @clobber A B R4:R5 R6:R7
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
        *** @clobber A B R4:R5 R6:R7
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
        push    R7               ; push remainder
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

        *** Multiply: result = multiplicand * multiplier
        *** The 16-bit product is the same for signed and unsigned operands.
        *** A high byte of >FF, a sign-extended byte, contributes
        *** -lo(other operand) to hi(result) without an MPY.
        *** @param R4:R5 multiplicand
        *** @param R6:R7 multiplier
        *** @return R4:R5 result
        *** @clobber A B
mul16:
umul16:
        mov     R4, A
        or      R5, A
        jz      umul16_end      ; branch if multiplicand == 0
        mov     R6, A
        or      R7, A
        jz      umul16_zero     ; branch if multiplier == 0
        mov     R4, A
        jz      umul16_hi       ; branch if hi(multiplicand) == 0
        inc     A
        jnz     umul16_hi_mpy   ; branch if hi(multiplicand) != >FF
        sub     R7, A
        mov     A, R4           ; R4=-lo(multiplier)
umul16_hi:
        mov     R6, A
        jz      umul16_lo       ; branch if hi(multiplier) == 0
        inc     A
        jnz     umul16_lo_mpy   ; branch if hi(multiplier) != >FF
        sub     R5, R4          ; R4-=lo(multiplicand)
umul16_lo:
        mpy     R5, R7          ; A:B=lo(multiplicand)*lo(multiplier)
        add     A, R4
        mov     B, R5           ; R4:R5=result
umul16_end:
        rets
umul16_hi_mpy:
        mpy     R7, R4          ; A:B=hi(multiplicand)*lo(multiplier)
        mov     B, R4           ; R4=hi(result)
        jmp     umul16_hi
umul16_lo_mpy:
        mpy     R5, R6          ; A:B=lo(multiplicand)*hi(multiplier)
        add     B, R4           ; R4=hi(result)
        jmp     umul16_lo
umul16_zero:
        clr     R4
        clr     R5              ; R4:R5=0
        rets

        *** Unsigned division: dividend / divisor = quotient ... remainder
        *** @param R4:R5 dividend
        *** @param R6:R7 divisor
        *** @return R4:R5 quotient
        *** @return R6:R7 remainder
        *** @clobber A B
udiv16:
        cmp     R6, R4
        jl      udiv16_small    ; branch if dividend < divisor
        jne     udiv16_divide
        cmp     R7, R5
        jhs     udiv16_divide
udiv16_small:
        movd    R5, R7          ; R6:R7=remainder=dividend
        clr     R4
        clr     R5              ; R4:R5=quotient=0
        rets
udiv16_divide:
        mov     R6, A
        jnz     udiv16_word     ; branch if divisor >= 256
        mov     R4, A           ; A=hi(dividend)
        jz      udiv16_check    ; branch if dividend < 256
        cmp     R7, A
        jhs     udiv16_check    ; branch if quotient >= 256 or divisor==0
        *** The quotient fits in a byte: shift lo(dividend) through the
        *** remainder A, one quotient bit per step.
        clr     R4              ; R4=hi(quotient)=0
        mov     R7, B           ; B=divisor
        push    R8
        mov     %8, R8          ; R8=bits
udiv16_byte_loop:
        rlc     R5              ; shift a quotient bit in, a dividend bit out
        rlc     A               ; into the remainder
        jc      udiv16_byte_over ; remainder > 255 > divisor
        cmp     B, A
        jl      udiv16_byte_next ; C=0 if remainder < divisor
        sub     B, A            ; C=1
udiv16_byte_next:
        djnz    R8, udiv16_byte_loop
        rlc     R5              ; shift the last quotient bit in
        mov     A, R7           ; R6:R7=remainder
        pop     R8
        rets
udiv16_byte_over:
        sub     B, A            ; C=0
        setc
        jmp     udiv16_byte_next
udiv16_check:
        mov     R7, A
        jz      udiv16_end      ; branch if divisor==0
udiv16_word:
        push    R8
        mov     %1, R8          ; R8=bits
        mov     R4, A
        mov     R5, B
        clrc
        rrc     A
        rrc     B               ; A:B=dividend>>1
        jmp     udiv16_prep
udiv16_prep_loop:
        clrc
        rlc     R7
        rlc     R6              ; divisor <<= 1
        inc     R8              ; ++bits
udiv16_prep:
        cmp     R6, A
        jl      udiv16_prep_end ; until divisor > dividend>>1
        jne     udiv16_prep_loop
        cmp     R7, B
        jhs     udiv16_prep_loop
udiv16_prep_end:
        clr     A
        clr     B               ; A:B=quotient=0
udiv16_loop:
        sub     R7, R5
        sbb     R6, R4          ; dividend -= divisor
        jhs     udiv16_shift    ; C=1 sets the quotient bit
        add     R7, R5
        adc     R6, R4          ; dividend += divisor
        clrc
udiv16_shift:
        rlc     B
        rlc     A               ; quotient <<= 1, C=0
        rrc     R6
        rrc     R7              ; divisor >>= 1
        djnz    R8, udiv16_loop ; while --bits != 0
        movd    R5, R7          ; R6:R7=remainder
        mov     A, R4
        mov     B, R5           ; R4:R5=quotient
        pop     R8
udiv16_end:
        rets

        *** Division: dividend / divisor = quotient ... remainder
        *** @param R4:R5 dividend
        *** @param R6:R7 divisor
        *** @return R4:R5 quotient
        *** @return R6:R7 remainder
        *** @clobber A B
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
