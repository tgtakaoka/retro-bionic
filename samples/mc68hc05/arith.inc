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
        ora     #'0'
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
        ldx     #R1
        bsr     push_RX         ; push R1
        ldx     #R2
        bsr     push_RX         ; push R2
        lda     R0H
        bpl     print_int16_print
        lda     #'-'
        jsr     putchar
        ldx     #R0
        bsr     negate_RX
print_int16_print:
        bsr     print_uint16
        ldx     #R2
        bsr     pop_RX          ; pop R2
        ldx     #R1
        bra     pop_RX          ; pop R1

;;; Push register
;;; @param X register
;;; @clobber A
push_RX:
        stx     arith_work      ; save X
        lda     0,X             ; high(register)
        ldx     SP
        sta     stack,X         ; push high(register)
        ldx     arith_work
        lda     1,X             ; low(register)
        ldx     SP
        sta     stack+1,X       ; push low(register)
        incx
        incx
        stx     SP              ; update stack pointer
        ldx     arith_work      ; restore X
        rts

;;; Pop register
;;; @param X register
;;; @clobber A
pop_RX:
        stx     arith_work      ; save X
        ldx     SP
        decx
        decx
        stx     SP              ; update stack pointer
        lda     stack,X         ; pop high(register)
        ldx     arith_work
        sta     0,X             ; high(register)
        ldx     SP
        lda     stack+1,X       ; pop low(register)
        ldx     SP
        ldx     arith_work      ; restore X
        sta     1,X             ; low(register)
        rts

;;; Negate RX
;;; @param X register
negate_RX:
        clra
        sub     1,x
        sta     1,x
        clra
        sbc     0,x
        sta     0,x
        rts

;;; Set value to R0
;;; @param X high(value)
;;; @param A low(value)
set_R0:
        stx     R0H
        sta     R0L
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

;;; Load register to R0
;;; @param X register
;;; @clobber A
load_R0:
        lda     0,x
        sta     R0H
        lda     1,x
        sta     R0L
        rts

;;; Load register to R1
;;; @param X register
;;; @clobber A
load_R1:
        lda     0,x
        sta     R1H
        lda     1,x
        sta     R1L
        rts

;;; Load register to R2
;;; @param X register
;;; @clobber A
load_R2:
        lda     0,x
        sta     R2H
        lda     1,x
        sta     R2L
        rts

;;; store R0 to register
;;; @param X register
;;; @clobber A
store_R0:
        lda     R0H
        sta     0,x
        lda     R0L
        sta     1,x
        rts

;;; Addition: R0 = R1 + R2
;;; @clobber A
add16:
        lda     R1L
        add     R2L
        sta     R0L
        lda     R1H
        adc     R2H
        sta     R0H
        rts

;;; Subtraction: R0 = R1 + R2
;;; @clobber A
sub16:
        lda     R1L
        sub     R2L
        sta     R0L
        lda     R1H
        sbc     R2H
        sta     R0H
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
umul16:
        lda     R2H             ; A=high(multiplier)
        ldx     R1L             ; X=low(multiplicand)
        mul                     ; X:A=low(multiplicand)*high(multiplier)
        sta     R0H             ; high(result)=A
        lda     R2L             ; A=low(multiplier)
        ldx     R1H             ; X=high(multiplicand)
        mul                     ; X:A=high(multiplicand)*low(multiplier)
        add     R0H
        sta     R0H             ; high(result)+=A
        lda     R2L             ; A=low(multiplier)
        ldx     R1L             ; X=low(multiplicand)
        mul                     ; X:A=result
        sta     R0L             ; low(result)=A
        txa
        add     R0H
        sta     R0H             ; high(result)+=X
        rts

;;; Multiply; result = multiplicand * multiplier
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 result
;;; @clobber R1 R2
mul16:
        lda     R1H
        eor     R2H
        sta     arith_work      ; save hi(multiplicand^multiplier)
        lda     R2H             ; A=hi(multiplier)
        bpl     mul16_multiplicand
        ldx     #R2
        jsr     negate_RX       ; negate multiplier
mul16_multiplicand:
        lda     R1H             ; A=hi(multiplicand)
        bpl     mul16_multiply
        ldx     #R1
        jsr     negate_RX       ; negate multiplicand
mul16_multiply:
        bsr     umul16          ; R0=result
        lda     arith_work      ; A=(multiplicand^multiplier)
        bpl     mul16_end
        ldx     #R0
        jsr     negate_RX       ; negate result
mul16_end:
        rts

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram R1 dividend
;;; @praram R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber R2
udiv16:
        lda     R2H
        ora     R2L
        beq     udiv16_end      ; branch if R2==0
        lda     #1              ; A=bits
        sta     arith_work      ; bits=1
        bra     udiv16_prep
udiv16_prep_loop:
        lsl     R2L
        rol     R2H             ; divisor <<= 1
        inc     arith_work      ; ++bits
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
        dec     arith_work      ; --bits
        bne     udiv16_loop     ; while bits != 0
udiv16_end:
        rts

;;; Division; dividend / divisor = quotient ... remainder
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber R2
div16:
        lda     R1H
        eor     R2H
        sta     div16_sign      ; save hi(dividend^divisor)
        lda     R2H             ; A=hi(divisor)
        bpl     div16_dividend
        ldx     #R2
        jsr     negate_RX       ; negate divisor
div16_dividend:
        lda     R1H             ; A=hi(dividend)
        bpl     div16_divide
        ldx     #R1
        jsr     negate_RX       ; negate dividend
div16_divide:
        jsr     udiv16          ; R0=quotient
        lda     div16_sign      ; A=(dividend^divisor)
        bpl     div16_end
        ldx     #R0
        jsr     negate_RX       ; negate result
div16_end:
        rts
div16_sign:
        rmb     1
