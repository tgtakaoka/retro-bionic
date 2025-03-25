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
;;; @clobber R0 R1 R2
umul16:
        clr     R0H
        clr     R0L
        bra     umul16_check
umul16_loop:
        lsr     R2H
        ror     R2L             ; multiplier >>=1
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
umul16_check:
        lda     R2H
        ora     R2L
        bne     umul16_loop     ; while multiplier != 0
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

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram R1 dividend
;;; @praram R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber R2
udiv16:
        stx     arith_work
        lda     R2H
        ora     R2L
        beq     udiv16_end      ; branch if R2==0
        ldx     #1              ; X=bits
        bra     udiv16_prep
udiv16_prep_loop:
        lsl     R2L
        rol     R2H             ; divisor <<= 1
        incx                    ; ++bits
udiv16_prep:
        lda     R2H
        bpl     udiv16_prep_loop ; while msb(divisor) == 0
        clr     R0H
        clr     R0L             ; quotient=0
        bra     udiv16_enter_loop
udiv16_loop:
        lsr     R2H
        ror     R2L             ; divisor >>= 1
        lsl     R0L
        rol     R0H             ; quotient <<= 1
udiv16_enter_loop:
        lda     R1L
        sub     R2L
        sta     R1L
        lda     R1H
        sbc     R2H
        sta     R1H             ; dividend -= divisor
        bcs     udiv16_readd
        inc     R0L             ; quotient |= 1
        bra     udiv16_next
udiv16_readd:
        lda     R1L
        add     R2L
        sta     R1L
        lda     R1H
        adc     R2H
        sta     R1H             ; dividend += divisor
udiv16_next:
        decx                    ; --bits
        bne     udiv16_loop     ; while bits != 0
udiv16_end:
        ldx     arith_work
        rts

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
