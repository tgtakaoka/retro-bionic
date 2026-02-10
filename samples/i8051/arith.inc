;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8051

;;; Print signed 16-bit integer as decimal
;;; @param R0 &value
;;; @clobber A
print_int16:
        mov     A, R0
        push    ACC             ; save R0
        mov     A, @R0
        xch     A, R2
        push    ACC             ; save R2
        inc     R0
        mov     A, @R0
        xch     A, R3
        push    ACC             ; save R3
        mov     A, R3
        jnb     ACC.7, print_uint16
        mov     A, #'-'
        acall   putchar         ; print '-'
        acall   neg_R3R2        ; negate value
;;; Print unsigned 16-bit integer as decimal
;;; @param R3:R2 value
;;; @param print_uint16_buf external data memory
;;; @clobber A
print_uint16:
        mov     DPTR, #print_uint16_buf
        clr     A
        movx    @DPTR, A        ; mark EOS
print_uint16_loop:
        mov     R4, #10
        acall   udiv16_8        ; R2:R3/10=R2:R3...R4
        mov     A, R4           ; lo(remainder)
        add     A, #'0'
        inc     DPTR
        movx    @DPTR, A        ; write to buffer
        mov     A, R2
        orl     A, R3
        jnz     print_uint16_loop ; until value == 0
print_uint16_out:
        movx    A, @DPTR
        jz      print_uint16_end
        acall   putchar
        dec     DPL
        sjmp    print_uint16_out
print_uint16_end:
        pop     ACC
        mov     R3, A           ; restore R3
        pop     ACC
        mov     R2, A           ; restore R2
        pop     ACC
        mov     R0, A           ; restore R0
        ret

;;; Negation; result = -result
;;; @param R3:R2 result
;;; @clobber A
neg_R3R2:
        clr     A
        clr     C
        subb    A, R2
        mov     R2, A
        clr     A
        subb    A, R3
        mov     R3, A
        ret

;;; Negation; result = -result
;;; @param R5:R4 result
;;; @clobber A
neg_R5R4:
        clr     A
        clr     C
        subb    A, R4
        mov     R4, A
        clr     A
        subb    A, R5
        mov     R5, A
        ret

;;; Negation; result = -value
;;; @param R2: &result
;;; @param R3: &value
;;; @clobber A R0 R2 R3
negsi2:
        mov     A, R3
        mov     R0, A           ; R0=&value
        clr     A
        clr     C
        subb    A, @R0          ; A=lo(-value)
        mov     R3, A           ; R3=lo(-value)
        inc     R0
        clr     A
        subb    A, @R0          ; A=hi(-value)
        xch     A, R2           ; R2=hi(-value)
        mov     R0, A           ; R0=&result
        mov     A, R3
        mov     @R0, A          ; store lo(-value)
        inc     R0
        mov     A, R2
        mov     @R0, A          ; store hi(-value)
        ret

;;; Signed addition: summand += addend
;;; @param R2: &summand
;;; @param R3: &addend
;;; @clobber A R2 R3
addsi2:
        mov     A, R3
        mov     R0, A           ; R0=&addend
        mov     A, @R0          ; A=lo(addend)
        mov     R3, A           ; R3=lo(addend)
        inc     R0
        mov     A, @R0          ; A=hi(addend)
        xch     A, R2           ; R2=hi(addend)
        mov     R0, A           ; R0=&summand
        mov     A, @R0          ; A=lo(summand)
        add     A, R3           ; A=lo(summand+addend)
        mov     @R0, A
        inc     R0
        mov     A, @R0          ; A=hi(summand)
        addc    A, R2           ; A=hi(summand+addend)
        mov     @R0, A
        ret

;;; Singed subtraction: minuend -= subtrahend
;;; @param R2: &minuend
;;; @param R3: &subtrahend
;;; @clobber A R0 R2 R3
subsi2:
        mov     A, R3
        mov     R0, A           ; R0=&subtrahend
        mov     A, @R0          ; A=lo(subtrahend)
        mov     R3, A           ; R3=lo(subtrahend)
        inc     R0
        mov     A, @R0          ; A=hi(subtrahend)
        xch     A, R2           ; R2=hi(subtrahend)
        mov     R0, A           ; R0=&minuend
        mov     A, @R0          ; A=lo(minuend)
        clr     C
        subb    A, R3           ; A=lo(minuend-subtrahend)
        mov     @R0, A
        inc     R0
        mov     A, @R0          ; A=hi(minuend)
        subb    A, R2           ; A=hi(minuend-subtrahend)
        mov     @R0, A
        ret

;;; Signed comparison: minuend - subtrahend
;;; @param R2: &minuend
;;; @param R3: &subtrahend
;;; @return A  0: minuend == subtrahend
;;;         A  1: minuend >  subtrahend
;;;         A -1: minuend <  subtrahend
;;; @clobber A R0 R2 R3
cmpsi2:
        mov     A, R3
        mov     R0, A           ; R0=&subtrahend
        mov     A, @R0          ; A=lo(subtrahend)
        mov     R3, A           ; R3=lo(subtrahend)
        inc     R0
        mov     A, @R0          ; A=hi(subtrahend)
        push    ACC             ; save hi(subtrahend)
        xch     A, R2           ; R2=hi(subtrahend)
        mov     R0, A           ; R0=&minuend
        mov     A, @R0          ; A=lo(minuend)
        clr     C
        subb    A, R3
        mov     R3, A           ; A=lo(minuend-subtrahend)
        inc     R0
        mov     A, @R0          ; A=hi(minuend)
        subb    A, R2
        mov     R2, A           ; R2=hi(minuend-subtrahend)
        orl     A, R3
        jz      cmpsi2_eq       ; branch if minuend-subtrahend==0
        pop     ACC             ; restore hi(subtrahend)
        xrl     A, @R0          ; A=hi(minuend^subtrahend)
        push    ACC             ; save hi(minuend^subtrahend)
        mov     A, R2
        xrl     A, @R0
        mov     R3, A           ; R3=hi(minuend-subtrahend)^minuend
        pop     ACC             ; restore hi(minuend^subtrahend)
        anl     A, R3           ; overflow flag
        xrl     A, R2
        jb      ACC.7, cmpsi2_lt
cmpsi2_gt:
        mov     A, #1
        ret
cmpsi2_lt:
        mov     A, #-1
        ret
cmpsi2_eq:
        dec     SP              ; discard hi(subtrahend)
        ret

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param R3:R2 multiplicand
;;; @param R5:R4 multiplier
;;; @return R3:R2 result
;;; @clobber A R2 R3 R6 R7
umul16:
        mov     A, R2           ; A=lo(multiplicand)
        mov     B, R4           ; B=lo(multiplier)
        mul     AB              ; B:A
        mov     R6, A
        mov     R7, B           ; R7:R6=result
        mov     A, R2           ; A=lo(multiplicand)
        mov     B, R5           ; B=hi(multiplier)
        mul     AB
        add     A, R7
        mov     R7, A           ; result+=lo(BA)
        mov     A, R3           ; A=hi(multiplicand)
        mov     B, R4           ; B=lo(multiplier)
        mul     AB
        add     A, R7
        mov     R3, A
        mov     A, R6
        mov     R2, A
        ret

;;; Signed multiplication: multiplicand *= multiplier
;;; @param R2: &multiplicand
;;; @param R3: &multiplier
;;; @clobber A R0 R2 R3 R4 R5 R6 R7
mulsi2:
        mov     A, R2
        push    ACC             ; save &multiplicand
        mov     A, R3
        push    ACC             ; save &multiplier
        mov     R0, A           ; R0=&multiplier
        mov     A, @R0
        mov     R4, A
        inc     R0
        mov     A, @R0
        mov     R5, A           ; R5:R4=multiplier
        jnb     ACC.7, mulsi2_multiplicand
        acall   neg_R5R4
mulsi2_multiplicand:
        mov     A, R2
        mov     R0, A           ; R0=&multiplicand
        mov     A, @R0
        mov     R2, A
        inc     R0
        mov     A, @R0
        mov     R3, A
        jnb     ACC.7, mulsi2_multiply
        acall   neg_R3R2
mulsi2_multiply:
        acall   umul16
        pop     ACC
        mov     R0, A           ; R0=&multiplier
        inc     R0
        mov     A, @R0          ; A=hi(multiplier)
        mov     R5, A           ; R5=hi(multiplier)
        pop     ACC
        mov     R0, A           ; R0=&multiplicand
        inc     R0
        mov     A, @R0          ; A=hi(multiplicand)
        xrl     A, R5           ; A=hi(multiplicand^multiplier)
        jnb     ACC.7, mulsi2_store
        acall   neg_R3R2
mulsi2_store:
        mov     A, R3
        mov     @R0, A
        dec     R0
        mov     A, R2
        mov     @R0, A
        ret

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @param R3:R2 dividend
;;; @param R5:R4 divisor
;;; @return R3:R2 quotient
;;; @return R5:R4 remainder
;;; @clobber A R1 R2 R3 R4 R5 R6 R7
udiv16:
        mov     A, R5
        jnz     udiv16_large    ; branch if divisor >= 256
        orl     A, R4
        jz      udiv16_zero     ; divide by zero
        mov     A, R3
        jz      udiv8_8         ; branch if divisor < 256
;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @param R3:R2 dividend
;;; @param R4 divisor
;;; @return R3:R2 quotient
;;; @return R4 remainder
;;; @clobber A B R1
udiv16_8:
        mov     A, R3           ; high(dividend)
        mov     B, R4           ; divisor
        div     AB              ; A=high(quotient), B=reminder of high
        mov     R3, A           ; R3=high(quotient)
        ;; Divide B:R2 by R4 where B<R4
        mov     R1, #8          ; loop counter
udiv16_8_loop:
        mov     A, R2
        clr     C
        rlc     A
        mov     R2, A
        mov     A, B
        rlc     A
        mov     B, A            ; B:R2 <<= 1
        ;; Compare B with R4 (divisor)
        mov     A, B
        clr     C
        subb    A, R4           ; A=B-R4
        jc      udiv16_8_next   ; jump if B < R4
        mov     B, A            ; update B
        inc     R2              ; set lowest bit as part of low(quotient)
udiv16_8_next:
        djnz    R1, udiv16_8_loop
        mov     R4, B           ; R4=reminder
        ret
;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @param R2 dividend
;;; @param R4 divisor
;;; @return R2 quotient
;;; @return R4 remainder
;;; @clobber A B
udiv8_8:
        mov     A, R2
        mov     B, R4           ; A=dividend, B=divisor
        div     AB              ; A=quotient, B=reminder
        mov     R2, A
        mov     R4, B
        ret
udiv16_zero:
        mov     A, R3           ; remainder = dividend
        mov     R5, A
        mov     A, R2
        mov     R4, A
        clr     A               ; quotient = 0
        mov     R3, A
        mov     R2, A
        ret
;;; dividend > divisor
udiv16_large:
        mov     A, R3
        clr     C
        subb    A, R5
        jc      udiv16_zero     ; branch if dividend < divisor
        jnz     udiv16_estimate ; branch if dividend > divisor
        mov     A, R2           ; compare lower byte
        clr     C
        subb    A, R4
        jc      udiv16_zero     ; branch if dividend < divisor
udiv16_estimate:
        mov     A, R3
        mov     B, R5
        div     AB              ; A=estimated quotient
        mov     R1, A           ; R1=estimated quotient
udiv16_refine:
        mov     A, R1
        mov     B, R4
        mul     AB              ; B:A=estimated*low(divisor)
        mov     R6, A           ; R6=low-low(product)
        mov     R7, B           ; R7=low-high(product)
        mov     A, R1
        mov     B, R5
        mul     AB              ; B:A=estimate*high(divisor)
        add     A, R7
        mov     R7, A           ; R7+=mid(product)
        mov     A, R2           ; dividend-product
        clr     C
        subb    A, R6
        mov     A, R3
        subb    A, R7
        jnc     udiv16_end
        dec     R1              ; decrease Q
        sjmp    udiv16_refine
udiv16_end:
        mov     A, R2
        clr     C
        subb    A, R6
        mov     R4, A           ; low(remainder)
        mov     A, R3
        subb    A, R7
        mov     R5, A           ; high(remainder)
        mov     A, R1
        mov     R2, A           ; low(quotient)
        mov     R3, #0          ; high(quotient)
        ret

;;; Signed division: dividend /= divisor
;;; @param R2: &dividend
;;; @param R3: &divisor
;;; @clobber A R0 R2 R3 R4 R5 R6 R7
divsi2:
        mov     A, R2
        push    ACC             ; save &dividend
        mov     A, R3
        push    ACC             ; save &divisor
        mov     R0, A           ; R0=&divisor
        mov     A, @R0
        mov     R4, A
        inc     R0
        mov     A, @R0
        mov     R5, A           ; R5:R4=divisor
        jnb     ACC.7, divsi2_dividend
        acall   neg_R5R4
divsi2_dividend:
        mov     A, R2
        mov     R0, A           ; R0=&dividend
        mov     A, @R0
        mov     R2, A
        inc     R0
        mov     A, @R0
        mov     R3, A
        jnb     ACC.7, divsi2_divide
        acall   neg_R3R2
divsi2_divide:
        acall   udiv16
        pop     ACC
        mov     R0, A           ; R0=&divisor
        inc     R0
        mov     A, @R0          ; A=hi(divisor)
        mov     R5, A           ; R5=hi(divisor)
        pop     ACC
        mov     R0, A           ; R0=&dividend
        inc     R0
        mov     A, @R0          ; A=hi(dividend)
        xrl     A, R5           ; A=hi(dividend^divisor)
        jnb     ACC.7, divsi2_store
        acall   neg_R3R2
divsi2_store:
        mov     A, R3
        mov     @R0, A
        dec     R0
        mov     A, R2
        mov     @R0, A
        ret
