;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8048

;;; Print signed 16-bit integer as decimal
;;; @param R0 &value
;;; @clobber A
print_int16:
        mov     A, R0
        dec     R1
        movx    @R1, A          ; save R0
        mov     A, @R0
        xch     A, R2
        dec     R1
        movx    @R1, A          ; save R2
        inc     R0
        mov     A, @R0
        xch     A, R3
        dec     R1
        movx    @R1, A          ; save R3
        mov     A, R3
        cpl     A
        jb      7, print_uint16
        mov     A, #'-'
        call    putchar         ; print '-'
        call    neg_R3R2        ; negate value
;;; Print unsigned 16-bit integer as decimal
;;; @param R2:R3: value
;;; @param print_uint16_buf external data memory
;;; @clobber A
print_uint16:
        mov     A, #print_uint16_buf
        dec     R1
        movx    @R1, A          ; save buffer pointer
        mov     R0, A
        clr     A
        movx    @R0, A          ; mark EOS
print_uint16_loop:
        mov     R4, #LOW(10)
        mov     R5, #HIGH(10)
        call    udiv16
        movx    A, @R1
        mov     R0, A           ; restore buffer pointer
        mov     A, R4           ; lo(remainder)
        add     A, #'0'
        inc     R0
        movx    @R0, A          ; write to buffer
        mov     A, R0
        movx    @R1, A          ; save buffer pointer
        mov     A, R2
        orl     A, R3
        jnz     print_uint16_loop ; until value == 0
print_uint16_out:
        movx    A, @R0
        jz      print_uint16_end
        call    putchar
        dec     R0
        jmp     print_uint16_out
print_uint16_end:
        inc     R1              ; discard buffer pointer
        movx    A, @R1
        inc     R1
        mov     R3, A           ; restore R3
        movx    A, @R1
        inc     R1
        mov     R2, A           ; restore R2
        movx    A, @R1
        inc     R1
        mov     R0, A           ; restore R0
        ret

        org     $+(256-LOW($))

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
        cpl     A
        add     A, #1           ; A=lo(-subtrahend)
        mov     R3, A           ; R3=lo(-subtrahend)
        inc     R0
        mov     A, @R0          ; A=hi(subtrahend)
        cpl     A
        addc    A, #0           ; A=hi(-subtrahend)
        xch     A, R2           ; R2=hi(-subtrahend)
        mov     R0, A           ; R0=&minuend
        mov     A, @R0          ; A=lo(minuend)
        add     A, R3           ; A=lo(minuend-subtrahend)
        mov     @R0, A
        inc     R0
        mov     A, @R0          ; A=hi(minuend)
        addc    A, R2           ; A=hi(minuend-subtrahend)
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
        cpl     A
        add     A, #1           ; A=lo(-subtrahend)
        mov     R3, A           ; R3=lo(-subtrahend)
        inc     R0
        mov     A, @R0          ; A=hi(subtrahend)
        dec     R1
        movx    @R1, A          ; save hi(subtrahend)
        cpl     A
        addc    A, #0           ; A=hi(-subtrahend)
        xch     A, R2           ; R2=hi(-subtrahend)
        mov     R0, A           ; R0=&minuend
        mov     A, @R0          ; A=lo(minuend)
        add     A, R3
        mov     R3, A           ; A=lo(minuend-subtrahend)
        inc     R0
        movx    A, @R1          ; restore hi(subtrahend)
        xrl     A, @R0          ; A=hi(minuend^subtrahend)
        movx    @R1, A          ; save hi(minuend^subtrahend)
        mov     A, @R0          ; A=hi(minuend)
        addc    A, R2
        mov     R2, A           ; R2=hi(minuend-subtrahend)
        orl     A, R3
        jz      cmpsi2_eq       ; branch if minuend-subtrahend==0
        mov     A, R2
        xrl     A, @R0
        mov     R3, A           ; R3=hi(minuend-subtrahend)^minuend
        movx    A, @R1          ; restore hi(minuend^subtrahend)
        inc     R1
        anl     A, R3           ; overflow flag
        xrl     A, R2
        jb      7, cmpsi2_lt
cmpsi2_gt:
        mov     A, #1
        ret
cmpsi2_lt:
        mov     A, #-1
        ret
cmpsi2_eq:
        inc     R1              ; discard hi(subtrahend)
        ret

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param R3:R2 multiplicand
;;; @param R5:R4 multiplier
;;; @return R3:R2 result
;;; @clobber A R2 R3 R4 R5 R6 R7
umul16:
        clr     A
        mov     R6, A
        mov     R7, A           ; R7:R6=result
        jmp     umul16_check
umul16_loop:
        clr     C
        mov     A, R5
        rrc     A
        mov     R5, A
        mov     A, R4
        rrc     A
        mov     R4, A           ; multiplier >>= 1
        jnc     umul16_next     ; branch if lsb(multiplier)==0
        mov     A, R6
        add     A, R2
        mov     R6, A
        mov     A, R7
        addc    A, R3
        mov     R7, A           ; result += multiplicand
umul16_next:
        clr     C
        mov     A, R2
        rlc     A
        mov     R2, A
        mov     A, R3
        rlc     A
        mov     R3, A           ; multiplicand <<= 1
umul16_check:
        mov     A, R4
        orl     A, R5
        jnz     umul16_loop     ; branch if multiplier != 0
umul16_end:
        mov     A, R6
        mov     R2, A
        mov     A, R7
        mov     R3, A
        ret

;;; Signed multiplication: multiplicand *= multiplier
;;; @param R2: &multiplicand
;;; @param R3: &multiplier
;;; @clobber A R0 R2 R3 R4 R5 R6 R7
mulsi2:
        mov     A, R2
        dec     R1
        movx    @R1, A          ; save &multiplicand
        mov     A, R3
        dec     R1
        movx    @R1, A          ; save &multiplier
        mov     R0, A           ; R0=&multiplier
        mov     A, @R0
        mov     R4, A
        inc     R0
        mov     A, @R0
        mov     R5, A           ; R5:R4=multiplier
        cpl     A
        jb      7, mulsi2_multiplicand
        call    neg_R5R4
mulsi2_multiplicand:
        mov     A, R2
        mov     R0, A           ; R0=&multiplicand
        mov     A, @R0
        mov     R2, A
        inc     R0
        mov     A, @R0
        mov     R3, A
        cpl     A
        jb      7, mulsi2_multiply
        call    neg_R3R2
mulsi2_multiply:
        call    umul16
        movx    A, @R1
        inc     R1
        mov     R0, A           ; R0=&multiplier
        inc     R0
        mov     A, @R0          ; A=hi(multiplier)
        mov     R5, A           ; R5=hi(multiplier)
        movx    A, @R1
        inc     R1
        mov     R0, A           ; R0=&multiplicand
        inc     R0
        mov     A, @R0          ; A=hi(multiplicand)
        xrl     A, R5           ; A=hi(multiplicand^multiplier)
        cpl     A
        jb      7, mulsi2_store
        call    neg_R3R2
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
;;; @clobber A R0 R2 R3 R4 R5 R6 R7
udiv16:
        mov     A, R4
        orl     A, R5
        jnz     udiv16_calc     ; branch if divisor != 0
        ret
udiv16_calc:
        mov     R0, #1          ; R0=bits
        jmp     udiv16_prep
udiv16_prep_loop:
        clr     C
        mov     A, R4
        rlc     A
        mov     R4, A
        mov     A, R5
        rlc     A
        mov     R5, A           ; divisor <<= 1
        inc     R0              ; bits++
udiv16_prep:
        mov     A, R5
        cpl     A
        jb      7, udiv16_prep_loop ; while msb(divisor) == 0
        clr     A
        dec     R1
        movx    @R1, A
        dec     R1
        movx    @R1, A          ; R1[2:1]=quotient
        mov     A, R1           ; A=&quotient
        xch     A, R0           ; R0=&quotient, A=bits
        dec     R1
        movx    @R1, A          ; R=&bits
        jmp     udiv16_enter_loop
udiv16_loop:
        clr     C
        mov     A, R5
        rrc     A
        mov     R5, A
        mov     A, R4
        rrc     A
        mov     R4, A           ; divisor >>= 1
        clr     C
        movx    A, @R0
        rlc     A
        movx    @R0, A
        inc     R0
        movx    A, @R0
        rlc     A
        movx    @R0, A          ; quotient <<= 1
        dec     R0
udiv16_enter_loop:
        mov     A, R4
        cpl     A
        add     A, #1
        mov     R6, A
        mov     A, R5
        cpl     A
        addc    A, #0
        mov     R7, A           ; R7:R6=-divisor
        mov     A, R2
        add     A, R6
        mov     R2, A
        mov     A, R3
        addc    A, R7
        mov     R3, A           ; divident -= divisor
        jnc     udiv16_readd    ; branch if dividend < 0
        movx    A, @R0
        inc     A               ; quotient |= 1
        movx    @R0, A
        jmp     udiv16_next
udiv16_readd:
        mov     A, R4
        add     A, R2
        mov     R2, A
        mov     A, R5
        addc    A, R3
        mov     R3, A           ; dividend += divisor
udiv16_next:
        movx    A, @R1
        dec     A
        movx    @R1, A          ; bits -= 1
        jz      udiv16_store
        jmp     udiv16_loop
udiv16_store:
        mov     A, R2
        mov     R4, A
        mov     A, R3
        mov     R5, A           ; store remainder
        movx    A, @R0
        mov     R2, A
        inc     R0
        movx    A, @R0
        mov     R3, A           ; store quotient
        inc     R1
        inc     R1
        inc     R1              ; discard bits and quotient
        mov     A, @R1
        ret


;;; Signed division: dividend /= divisor
;;; @param R2: &dividend
;;; @param R3: &divisor
;;; @clobber A R0 R2 R3 R4 R5 R6 R7
divsi2:
        mov     A, R2
        dec     R1
        movx    @R1, A          ; save &dividend
        mov     A, R3
        dec     R1
        movx    @R1, A          ; save &divisor
        mov     R0, A           ; R0=&divisor
        mov     A, @R0
        mov     R4, A
        inc     R0
        mov     A, @R0
        mov     R5, A           ; R5:R4=divisor
        cpl     A
        jb      7, divsi2_dividend
        call    neg_R5R4
divsi2_dividend:
        mov     A, R2
        mov     R0, A           ; R0=&dividend
        mov     A, @R0
        mov     R2, A
        inc     R0
        mov     A, @R0
        mov     R3, A
        cpl     A
        jb      7, divsi2_divide
        call    neg_R3R2
divsi2_divide:
        call    udiv16
        movx    A, @R1
        inc     R1
        mov     R0, A           ; R0=&divisor
        inc     R0
        mov     A, @R0          ; A=hi(divisor)
        mov     R5, A           ; R5=hi(divisor)
        movx    A, @R1
        inc     R1
        mov     R0, A           ; R0=&dividend
        inc     R0
        mov     A, @R0          ; A=hi(dividend)
        xrl     A, R5           ; A=hi(dividend^divisor)
        cpl     A
        jb      7, divsi2_store
        call    neg_R3R2
divsi2_store:
        mov     A, R3
        mov     @R0, A
        dec     R0
        mov     A, R2
        mov     @R0, A
        ret
