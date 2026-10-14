;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6502

;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber R0 R1 R2
print_uint16:
        lda     R0H
        ora     R0L
        beq     print_uint16_digit ; branch if value==0
print_uint16_inner:
        lda     R0H
        ldx     R0L
        sta     R1H
        stx     R1L
        ora     R0L
        beq     print_uint16_exit
        lda     #0
        ldx     #10
        sta     R2H
        stx     R2L             ; divisor=10
        jsr     udiv16          ; R0=value/10
        lda     R1L
        pha                     ; push remainder
        jsr     print_uint16_inner
        pla                     ; pop remainder
print_uint16_digit:
        ora     #'0'
        jmp     putchar
print_uint16_exit:
        rts

;;; Print signed 16-bit integer as decimal
;;; @param R0 value
;;; @clobber R0
print_int16:
        lda     R1H
        pha
        lda     R1L
        pha
        lda     R2H
        pha
        lda     R2L
        pha
        lda     R0H
        bpl     print_value
        lda     #'-'
        jsr     putchar
        ldx     #R0
        jsr     neg16           ; negate R0
print_value:
        jsr     print_uint16
        pla
        sta     R2L
        pla
        sta     R2H
        pla
        sta     R1L
        pla
        sta     R1H
        rts

;;; Set constant to R1
;;;     jsr     set_R1
;;;     .word   constant
;;; @clobber A Y
set_R1:
        ldx     #R1
        jmp     set_RX

;;; Set constant to R2
;;;     jsr     set_R2
;;;     .word   constant
;;; @clobber A Y
set_R2:
        ldx     #R2
        jmp     set_RX

;;; Set constant to RX
;;;     ldx     #Rx
;;;     jsr     set_RX
;;;     .word   constant
;;; @clobber A Y
set_RX:
        pla
        sta     arith_work
        pla
        sta     arith_work+1
        ldy     #1
        lda     (arith_work),Y
        sta     0,X
        iny
        lda     (arith_work),Y
        sta     1,X
        clc
        lda     arith_work
        adc     #3
        sta     arith_work
        lda     arith_work+1
        adc     #0
        sta     arith_work+1
        jmp     (arith_work)

;;; Store R0 to R1
store_R1:
        lda     R0L
        sta     R1L
        lda     R0H
        sta     R1H
        rts

;;; Store R0 to R2
store_R2:       
        lda     R0L
        sta     R2L
        lda     R0H
        sta     R2H
        rts

;;; Store R0 to variable
;;; @param R0
;;; @param X &variable
store_RX:
        lda     R0L
        sta     0,X
        lda     R0H
        sta     1,X
        rts

;;; Load variable to R0
;;; @param X &variable
;;; @return R0 variable
load_R0:
        lda     0,X
        sta     R0L
        lda     1,X
        sta     R0H
        rts

;;; Load variable to R1
;;; @param X &variable
;;; @return R1 variable
load_R1:
        lda     0,X
        sta     R1L
        lda     1,X
        sta     R1H
        rts

;;; Load variable to R2
;;; @param X &variable
;;; @return R2 variable
load_R2:
        lda     0,X
        sta     R2L
        lda     1,X
        sta     R2H
        rts

;;; Sign extend A into X
;;; @param A 8-bit value
;;; @return X:A sign extended 16-bit value
sex_A:  
        ldx     #0
        ora     #0
        bpl     sex_A_return
        ldx     #-1
sex_A_return:
        rts

;;; Negate
;;; @param X point to value
;;; @clobber A
neg16:
        sec
        lda     #0
        sbc     0,x
        sta     0,x
        lda     #0
        sbc     1,x
        sta     1,x
        rts

;;; Addition
;;; @param R1 summand
;;; @param R2 addend
;;; @return R0 R1+R2
addsi2:
        clc
        lda     R1L
        adc     R2L
        sta     R0L
        lda     R1H
        adc     R2H
        sta     R0H
        rts

;;; Subtraction
;;; @param R1 minuend
;;; @param R2 subtrahend
;;; @return R0 R1-R2
subsi2:
        sec
        lda     R1L
        sbc     R2L
        sta     R0L
        lda     R1H
        sbc     R2H
        sta     R0H
        rts

;;; Signed compare helper
;;; @param R0 minuend-subtrahend
;;; @param R1 minuend
;;; @param R2 subtrahend
;;; @return R0=0;  BZ (minuend == subtrahend)
;;;         R0=1;  BP (minuend > subtrahend)
;;;         R0=-1; BM (minuend < subtrahend)
cmpsi2:
        sec
        lda     R1L
        sbc     R2L
        sta     R0L
        lda     R1H
        sbc     R2H
        sta     R0H             ; R0=R1-R2
        ora     R0L
        beq     cmpsi2_return   ; return with R0=0
        lda     R0H
        bvs     cmpsi2_ov       ; branch if V=1
        bmi     cmpsi2_lt       ; branch if V=0, N=1
cmpsi2_gt:
        lda     #0              ; N^V=0
        sta     R0H
        lda     #1
        sta     R0L
        rts
cmpsi2_ov:
        bmi     cmpsi2_gt       ; branch if V=1, N=1
cmpsi2_lt:
        lda     #-1             ; N^V=1
        sta     R0H
        sta     R0L             ; R0=-1
cmpsi2_return:
        rts

;;; Multiply helper
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 multiplicand * multiplier
;;; @clobber A X Y R1 R2
;;; A 16-bit product is the same signed or unsigned; a negative
;;; multiplier is negated only to shorten the loop.
mulsi2:
        lda     R2H
        bpl     umul16          ; branch if multiplier >= 0
        sec
        lda     #0
        sbc     R2L
        sta     R2L
        lda     #0
        sbc     R2H
        sta     R2H             ; negate multiplier
        sec
        lda     #0
        sbc     R1L
        sta     R1L
        lda     #0
        sbc     R1H
        sta     R1H             ; negate multiplicand
                                ; fall through to umul16

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 result
;;; @clobber A X Y R1 R2
umul16:
        ldx     #0              ; X=lo(result)
        ldy     R2H             ; Y=hi(result)
        beq     umul16_lo       ; branch if hi(multiplier)==0
        lda     R1L
        pha                     ; save lo(multiplicand)
        txa                     ; A=hi(result)
;;; hi(multiplier) only adds lo(multiplicand) into hi(result)
umul16_hi_loop:
        lsr     R2H             ; hi(multiplier) >>= 1
        bcc     umul16_hi_next  ; branch if lsb(multiplier)==0
        clc
        adc     R1L             ; hi(result) += lo(multiplicand)
umul16_hi_next:
        asl     R1L             ; lo(multiplicand) <<= 1
        ldy     R2H
        bne     umul16_hi_loop  ; while hi(multiplier) != 0
        tay                     ; Y=hi(result)
        pla
        sta     R1L             ; restore lo(multiplicand)
umul16_lo:
        lda     R2L
        beq     umul16_end      ; branch if lo(multiplier)==0
        bne     umul16_lo_enter ; always branch
umul16_lo_loop:
        asl     R1L
        rol     R1H             ; multiplicand <<= 1
umul16_lo_enter:
        lsr     R2L             ; lo(multiplier) >>= 1
        bcc     umul16_lo_loop  ; branch if lsb(multiplier)==0
        beq     umul16_lo_last  ; branch if lo(multiplier)==0
        clc
        txa
        adc     R1L
        tax
        tya
        adc     R1H
        tay                     ; result += multiplicand
        jmp     umul16_lo_loop
umul16_lo_last:
        clc
        txa
        adc     R1L
        tax
        tya
        adc     R1H
        tay                     ; result += multiplicand
umul16_end:
        stx     R0L
        sty     R0H
        rts

;;; divsi2 with a negative divisor
divsi2_neg_divisor:
        sec
        lda     #0
        sbc     R2L
        sta     R2L
        lda     #0
        sbc     R2H
        sta     R2H             ; negate divisor
        lda     R1H
        bpl     divsi2_neg_quotient ; branch if dividend >= 0
        sec
        lda     #0
        sbc     R1L
        sta     R1L
        lda     #0
        sbc     R1H
        sta     R1H             ; negate dividend
        jmp     udiv16
;;; divsi2 with a negative dividend and a non-negative divisor
divsi2_neg_dividend:
        sec
        lda     #0
        sbc     R1L
        sta     R1L
        lda     #0
        sbc     R1H
        sta     R1H             ; negate dividend
divsi2_neg_quotient:
        jsr     udiv16
        sec
        lda     #0
        sbc     R0L
        sta     R0L
        lda     #0
        sbc     R0H
        sta     R0H             ; negate quotient
        rts

;;; Division helper
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber A X Y R2
;;; Non-negative operands fall through to udiv16.
divsi2:
        lda     R2H
        bmi     divsi2_neg_divisor ; branch if divisor < 0
        lda     R1H
        bmi     divsi2_neg_dividend ; branch if dividend < 0
                                ; fall through to udiv16

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber A X Y R2
udiv16:
        lda     R2H
        bne     udiv16_wide     ; branch if divisor >= 256
        lda     R2L
        beq     udiv16_return   ; branch if R2==0
        bmi     udiv16_wide     ; branch if divisor >= 128
        lda     R1H
        beq     udiv8           ; branch if dividend < 256
        cmp     R2L
        bcc     udiv16_8        ; branch if hi(dividend) < divisor
udiv16_wide:
        lda     #0
        sta     R0H
        sta     R0L             ; quotient=0
        ldx     #1              ; X=bits
        lda     R2H
        bmi     udiv16_enter_loop ; branch if msb(divisor) == 1
        lda     R1L
        cmp     R2L
        lda     R1H
        sbc     R2H
        bcc     udiv16_return   ; branch if dividend < divisor
udiv16_prep:
        inx                     ; ++bits
        asl     R2L
        rol     R2H             ; divisor <<= 1
        bmi     udiv16_enter_loop ; branch if msb(divisor) == 1
        lda     R1L
        cmp     R2L
        lda     R1H
        sbc     R2H
        bcs     udiv16_prep     ; while divisor <= dividend
        bcc     udiv16_next     ; always branch, C=0
udiv16_loop:
        lsr     R2H
        ror     R2L             ; divisor >>= 1
udiv16_enter_loop:
        lda     R1L
        sec
        sbc     R2L
        tay
        lda     R1H
        sbc     R2H             ; C=0 if dividend < divisor
        bcc     udiv16_next
        sta     R1H
        sty     R1L             ; dividend -= divisor
udiv16_next:
        rol     R0L
        rol     R0H             ; quotient = quotient<<1 | C
        dex                     ; --bits
        bne     udiv16_loop     ; while bits != 0
udiv16_return:
        rts

;;; udiv16 of a dividend less than 256 by a divisor less than 128
;;; @param A 0
udiv8:
        sta     R0H
        sta     R0L             ; quotient=0
        ldx     #1              ; X=bits
        lda     R1L             ; A=dividend
        cmp     R2L
        bcc     udiv8_return    ; branch if dividend < divisor
udiv8_prep:
        inx                     ; ++bits
        asl     R2L             ; divisor <<= 1
        bmi     udiv8_enter_loop ; branch if msb(divisor) == 1
        cmp     R2L
        bcs     udiv8_prep      ; while divisor <= dividend
        bcc     udiv8_next      ; always branch, C=0
udiv8_loop:
        lsr     R2L             ; divisor >>= 1
udiv8_enter_loop:
        cmp     R2L             ; C=0 if dividend < divisor
        bcc     udiv8_next
        sbc     R2L             ; dividend -= divisor
udiv8_next:
        rol     R0L             ; quotient = quotient<<1 | C
        dex                     ; --bits
        bne     udiv8_loop      ; while bits != 0
        sta     R1L             ; remainder
udiv8_return:
        rts

;;; udiv16 of a divisor less than 128 and greater than hi(dividend)
;;; @param A hi(dividend)
;;; @param P.C 0
udiv16_8:
        ldx     #8              ; X=bits
udiv16_8_loop:
        rol     R1L             ; shift in quotient bit
        rol     a               ; A:lo(dividend) <<= 1
        cmp     R2L             ; C=0 if A < divisor
        bcc     udiv16_8_next
        sbc     R2L             ; A -= divisor
udiv16_8_next:
        dex                     ; --bits
        bne     udiv16_8_loop   ; while bits != 0
        rol     R1L             ; shift in last quotient bit
        ldx     R1L
        stx     R0L             ; quotient
        sta     R1L             ; remainder
        lda     #0
        sta     R0H
        sta     R1H
        rts
