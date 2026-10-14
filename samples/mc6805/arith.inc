;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber A X R0 R1 R2
print_uint16:
        clrx
print_uint16_loop:
        lda     R0H
        sta     R1H
        lda     R0L
        sta     R1L             ; dividend = value
        ora     R0H
        beq     print_uint16_zero ; branch if value == 0
        clr     R2H
        lda     #10
        sta     R2L             ; divisor=10
        jsr     udiv16          ; R0=value/10
        lda     R1L
        ora     #'0'
        sta     print_uint16_buf,x
        incx
        bra     print_uint16_loop
print_uint16_zero:
        tstx
        bne     print_uint16_print
        lda     #'0'
        jmp     putchar
print_uint16_print:
        decx
        lda     print_uint16_buf,x
        jsr     putchar
        tstx
        bne     print_uint16_print
        rts
print_uint16_buf:
        rmb     8

;;; Print signed 16-bit integer as decimal
;;; @param R0 value
;;; @clobber A X R0
print_int16:
        ldx     R1H
        lda     R1L
        bsr     push_XA         ; push R1
        ldx     R2H
        lda     R2L
        bsr     push_XA         ; push R2
        lda     R0H
        bpl     print_int16_print
        lda     #'-'
        jsr     putchar
        lda     R0H
        nega
        neg     R0L
        sbc     #0
        sta     R0H
print_int16_print:
        bsr     print_uint16
        bsr     pull_XA         ; pull R2
        stx     R2H
        sta     R2L
        bsr     pull_XA         ; pull R1
        stx     R1H
        sta     R1L
        rts

;;; Set value to R1
;;; @param X high(value)
;;; @param A low(value)
set_R1:
        stx     R1H
        sta     R1L
        rts

;;; Set value to R2
;;; @param X high(value)
;;; @param A low(value)
set_R2:
        stx     R2H
        sta     R2L
        rts

;;; Push X:A
;;; @param X:A value
;;; @clobber X:A
push_XA:
        sta     arith_work      ; save A
        txa
        ldx     SP
        sta     stack,X         ; push high(value))
        lda     arith_work
        sta     stack+1,X       ; push low(register)
        incx
        incx
        stx     SP              ; update stack pointer
        rts

;;; Pull X:A
;;; @return X:A value
pull_XA:
        ldx     SP
        decx
        decx
        stx     SP              ; update stack pointer
        lda     stack+1,X       ; pull low(value)
        ldx     stack,X         ; pull high(value)
        rts

;;; Addition: R0 = R1 + R2
;;; @return X:A addition
;;; @clobber A
add16:
        lda     R1L
        add     R2L
        sta     R0L
        lda     R1H
        adc     R2H
        sta     R0H
        tax
        lda     R0L
        rts

;;; Subtraction: R0 = R1 + R2
;;; @return X:A subtraction
;;; @clobber A
sub16:
        lda     R1L
        sub     R2L
        sta     R0L
        lda     R1H
        sbc     R2H
        sta     R0H
        tax
        lda     R0L
        rts

;;; Signed compare R0=sign(R1-R2)
;;; @return R0=A=0;  BEQ (minuend == subtrahend)
;;;         R0=A=1;  BPL (minuend > subtrahend)
;;;         R0=A=-1; BMI (minuend < subtrahend)
;;; @clobber A
cmp16:
        bsr     sub16
        lda     R0H
        ora     R0L
        beq     cmp16_end       ; return with R0=0
        lda     R0H
        eor     R1H
        sta     R0L             ; R0L=(minuend-subtrahend)^minuend
        lda     R1H
        eor     R2H             ; minuend^subtrahend
        and     R0L             ; overflow flag
        eor     R0H
        bmi     cmp16_lt
cmp16_gt:
        clr     R0H
        lda     #1
        bra     cmp16_end
cmp16_lt:
        lda     #-1
        sta     R0H
cmp16_end:
        sta     R0L
        rts

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 result
;;; @clobber A X R0 R1 R2
umul16:
        clr     R0L
        clra                    ; A=high(result)
        ldx     R2H             ; X=high(multiplier)
        beq     umul16_low      ; branch if high(multiplier) == 0
;;; high(multiplier) only adds low(multiplicand) into high(result)
        lda     R1L
        sta     R2H             ; R2H=low(multiplicand)
        clra
umul16_high:
        lsrx                    ; high(multiplier) >>= 1
        bcc     umul16_high_next ; if lsb(multiplier) == 0
        add     R2H             ; high(result) += low(multiplicand)
umul16_high_next:
        lsl     R2H             ; low(multiplicand) <<= 1
        tstx
        bne     umul16_high     ; while high(multiplier) != 0
umul16_low:
        sta     R0H
        ldx     R2L             ; X=low(multiplier)
        beq     umul16_end      ; branch if low(multiplier) == 0
umul16_loop:
        lsrx                    ; low(multiplier) >>= 1
        bcc     umul16_next     ; if lsb(multiplier) == 0
        lda     R1L
        add     R0L
        sta     R0L
        lda     R1H
        adc     R0H
        sta     R0H             ; result += multiplicand
umul16_next:
        lsl     R1L
        rol     R1H             ; multiplicand <<= 1
        tstx
        bne     umul16_loop     ; while low(multiplier) != 0
umul16_end:
        rts

;;; Multiply; result = multiplicand * multiplier
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 result
;;; @return X:A result
;;; @clobber R1 R2
mul16:
        lda     R1H
        eor     R2H
        sta     arith_work      ; save hi(multiplicand^multiplier)
        lda     R2H             ; A=hi(multiplier)
        bpl     mul16_multiplicand
        nega
        neg     R2L
        sbc     #0
        sta     R2H             ; negate multiplier
mul16_multiplicand:
        lda     R1H             ; A=hi(multiplicand)
        bpl     mul16_multiply
        nega
        neg     R1L
        sbc     #0
        sta     R1H             ; negate multiplicand
mul16_multiply:
        bsr     umul16          ; R0=result
        lda     arith_work      ; A=(multiplicand^multiplier)
        bpl     mul16_end
        lda     R0H
        nega
        neg     R0L
        sbc     #0
        sta     R0H             ; negate result
mul16_end:
        ldx     R0H
        lda     R0L
        rts

;;; Unsigned 8-bit square; result = value * value
;;; @param X value
;;; @return A:X result, A high
;;; @clobber R1L
sqr8:
        stx     R1L             ; multiplicand=value

;;; Unsigned 8-bit multiplication: result = multiplicand * multiplier
;;; @param R1L multiplicand
;;; @param X multiplier
;;; @return A:X result, A high
umul8:
        clra                    ; hi(result)=0
        lsrx                    ; C=lsb(multiplier)
        bcc     umul8_1
        add     R1L             ; hi(result) += multiplicand
umul8_1:
        rora
        rorx                    ; result >>= 1, C=next multiplier bit
        bcc     umul8_2
        add     R1L             ; hi(result) += multiplicand
umul8_2:
        rora
        rorx                    ; result >>= 1, C=next multiplier bit
        bcc     umul8_3
        add     R1L             ; hi(result) += multiplicand
umul8_3:
        rora
        rorx                    ; result >>= 1, C=next multiplier bit
        bcc     umul8_4
        add     R1L             ; hi(result) += multiplicand
umul8_4:
        rora
        rorx                    ; result >>= 1, C=next multiplier bit
        bcc     umul8_5
        add     R1L             ; hi(result) += multiplicand
umul8_5:
        rora
        rorx                    ; result >>= 1, C=next multiplier bit
        bcc     umul8_6
        add     R1L             ; hi(result) += multiplicand
umul8_6:
        rora
        rorx                    ; result >>= 1, C=next multiplier bit
        bcc     umul8_7
        add     R1L             ; hi(result) += multiplicand
umul8_7:
        rora
        rorx                    ; result >>= 1, C=next multiplier bit
        bcc     umul8_8
        add     R1L             ; hi(result) += multiplicand
umul8_8:
        rora
        rorx                    ; result >>= 1
        rts

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber R2
udiv16:
        stx     udiv16_x        ; save X
        lda     R2H
        bne     udiv16_wide     ; branch if divisor >= 256
        lda     R1H
        beq     udiv16_wide     ; branch if dividend < 256
        cmp     R2L
        bcc     udiv16_wide     ; branch if quotient >= 256
;;; 8-bit quotient: A=remainder, shift the quotient into R1L
        ldx     #8              ; X=bits
udiv16_byte:
        rol     R1L             ; shift in ~quotient, shift out dividend
        rola                    ; remainder <<= 1
        bcs     udiv16_byte_over ; branch if remainder >= 256
        cmp     R2L
        bcs     udiv16_byte_next ; branch if remainder < divisor
        sub     R2L             ; remainder -= divisor
udiv16_byte_next:
        decx                    ; --bits
        bne     udiv16_byte     ; while bits != 0
        rol     R1L
        ldx     R1L
        comx
        stx     R0L
        clr     R0H             ; quotient
        sta     R1L
        clr     R1H             ; remainder
        bra     udiv16_end
udiv16_byte_over:
        sub     R2L             ; remainder -= divisor
        clc
        bra     udiv16_byte_next
udiv16_wide:
        lda     R2H
        ora     R2L
        beq     udiv16_end      ; branch if R2==0
        lda     #$FF
        sta     R0H
        sta     R0L             ; ~quotient=$FFFF
        clrx                    ; X=bits
udiv16_prep:
        lda     R1L
        sub     R2L
        lda     R1H
        sbc     R2H
        bcs     udiv16_prep_end ; until divisor > dividend
        incx                    ; ++bits
        lsl     R2L
        rol     R2H             ; divisor <<= 1
        bcc     udiv16_prep     ; while divisor < $10000
        ror     R2H
        bra     udiv16_shifted  ; divisor >>= 1, carry back in
udiv16_prep_end:
        tstx
        beq     udiv16_done     ; branch if quotient == 0
udiv16_loop:
        lsr     R2H
udiv16_shifted:
        ror     R2L             ; divisor >>= 1
        lda     R1L
        sub     R2L
        sta     arith_work
        lda     R1H
        sbc     R2H
        bcs     udiv16_next     ; branch if dividend < divisor
        sta     R1H
        lda     arith_work
        sta     R1L             ; dividend -= divisor
udiv16_next:
        rol     R0L
        rol     R0H             ; ~quotient = ~quotient<<1 | borrow
        decx                    ; --bits
        bne     udiv16_loop     ; while bits != 0
udiv16_done:
        com     R0L
        com     R0H             ; quotient
udiv16_end:
        ldx     udiv16_x        ; restore X
        rts
udiv16_x:
        rmb     1

;;; Division; dividend / divisor = quotient ... remainder
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R0 quotient
;;; @return X:A quotient
;;; @return R1 remainder
;;; @clobber R2
div16:
        lda     R1H
        eor     R2H
        sta     div16_sign      ; save hi(dividend^divisor)
        lda     R2H             ; A=hi(divisor)
        bpl     div16_dividend
        nega
        neg     R2L
        sbc     #0
        sta     R2H             ; negate divisor
div16_dividend:
        lda     R1H             ; A=hi(dividend)
        bpl     div16_divide
        nega
        neg     R1L
        sbc     #0
        sta     R1H             ; negate dividend
div16_divide:
        jsr     udiv16          ; R0=quotient
        lda     div16_sign      ; A=(dividend^divisor)
        bpl     div16_end
        lda     R0H
        nega
        neg     R0L
        sbc     #0
        sta     R0H             ; negate result
div16_end:
        ldx     R0H
        lda     R0L
        rts
div16_sign:
        rmb     1
