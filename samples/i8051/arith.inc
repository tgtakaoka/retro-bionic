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
;;; @param R2:R3: value
;;; @param print_uint16_buf external data memory
;;; @clobber A
print_uint16:
        mov     DPTR, #print_uint16_buf
        clr     A
        movx    @DPTR, A        ; mark EOS
print_uint16_loop:
        mov     R4, #LOW(10)
        mov     R5, #HIGH(10)
        acall   udiv16
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
        pop     AC
        mov     R2, A           ; restore R2
        pop     ACC
        mov     R0, A           ; restore R0
        ret

;;; Negation; result = -result
;;; @param R3:R2 result
;;; @clobber A
neg_R3R2:
        mov     A, R2
        cpl     A
        add     A, #1
        mov     R2, A
        mov     A, R3
        cpl     A
        addc    A, #0
        mov     R3, A
        ret

;;; Negation; result = -result
;;; @param R5:R4 result
;;; @clobber A
neg_R5R4:
        mov     A, R4
        cpl     A
        add     A, #1
        mov     R4, A
        mov     A, R5
        cpl     A
        addc    A, #0
        mov     R5, A
        ret

;;; Negation; result = -value
;;; @param R2: &result
;;; @param R3: &value
;;; @clobber A R0 R2 R3
negsi2:
        mov     A, R3
        mov     R0, A           ; R0=&value
        mov     A, @R0          ; A=lo(value)
        cpl     A
        add     A, #1
        mov     R3, A           ; R3=lo(-value)
        inc     R0
        mov     A, @R0          ; A=hi(value)
        cpl     A
        addc    A, #0           ; A=hi(-value)
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
;;; @praram R3:R2 dividend
;;; @praram R5:R4 divisor
;;; @return R3:R2 quotient
;;; @return R5:R4 remainder
;;; @clobber A R0 R1 R2 R3 R4 R5 R6 R7
udiv16:
        mov     A, R5
        jnz     udiv16_calc     ; branch if divisor >= 256
        orl     A, R4
        jz      udiv16_div0     ; B=divisor (<256)
        mov     B, A
        mov     A, R3
        jnz     udiv16_calc     ; branch if dividend >= 256
        mov     A, R2           ; A=dividend (<256)
        div     AB              ; A=quotient B=remainder
        mov     R2, A
        mov     R4, B
udiv16_div0:
        ret
udiv16_calc:
        mov     R1, #1          ; R1=bits
        sjmp    udiv16_prep
udiv16_prep_loop:
        clr     C
        mov     A, R4
        rlc     A
        mov     R4, A
        mov     A, R5
        rlc     A
        mov     R5, A           ; divisor <<= 1
        inc     R1              ; bits++
udiv16_prep:
        mov     A, R5
        jnb     ACC.7, udiv16_prep_loop ; while msb(divisor) == 0
        clr     A
        mov     R6, A
        mov     R7, A           ; R7:R6=quotient
        sjmp    udiv16_enter_loop
udiv16_loop:
        clr     C
        mov     A, R5
        rrc     A
        mov     R5, A
        mov     A, R4
        rrc     A
        mov     R4, A           ; divisor >>= 1
        clr     C
        mov     A, R6
        rlc     A
        mov     R6, A
        mov     A, R7
        rlc     A
        mov     R7, A           ; quotient <<= 1
udiv16_enter_loop:
        mov     A, R2
        clr     C
        subb    A, R4
        mov     R2, A
        mov     A, R3
        subb    A, R5
        mov     R3, A           ; divident -= divisor
        jc      udiv16_readd    ; branch if dividend < 0
        inc     R6              ; quotient |= 1
        sjmp    udiv16_next
udiv16_readd:
        mov     A, R2
        add     A, R4
        mov     R2, A
        mov     A, R3
        addc    A, R5
        mov     R3, A           ; dividend += divisor
udiv16_next:
        djnz    R1, udiv16_loop
        mov     A, R2
        mov     R4, A
        mov     A, R3
        mov     R5, A           ; store remainder
        mov     A, R6
        mov     R2, A
        mov     A, R7
        mov     R3, A           ; store quotient
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
