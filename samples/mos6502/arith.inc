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
        pha                     ; push reminder
        jsr     print_uint16_inner
        pla                     ; pop reminder
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
;;; @return R0 R1+R2
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

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 result
;;; @clobber R0 R1 R2
umul16:
        lda     #0
        sta     R0H
        sta     R0L
        beq     umul16_check    ; always branch
umul16_loop:
        lda     R2L
        and     #1
        beq     umul16_sr
        lda     R1L
        clc
        adc     R0L
        sta     R0L
        lda     R1H
        adc     R0H
        sta     R0H             ; result += multiplicand
umul16_sr:
        lsr     R2H
        ror     R2L             ; multiplier >>= 1
        asl     R1L
        rol     R1H             ; multilplicand <<= 1
umul16_check:
        lda     R2H
        ora     R2L
        bne     umul16_loop     ; while multiplier != 0
umul16_end:
        rts

;;; Multiply helper
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 multiplicand * multiplier
;;; @clobber R1 R2
mulsi2:
        lda     R1H
        eor     R2H
        pha                     ; save hi(multiplicand^multiplier)
        lda     R2H             ; A=hi(multiplier)
        bpl     mulsi2_multiplicand
        ldx     #R2
        jsr     neg16           ; negate multiplier
mulsi2_multiplicand:
        lda     R1H             ; A=hi(multiplicand)
        bpl     mulsi2_multiply
        ldx     #R1
        jsr     neg16           ; negate multiplicand
mulsi2_multiply:
        jsr     umul16          ; R0=result
        pla                     ; A=(multiplicand^multiplier)
        bpl     mulsi2_return
        ldx     #R0
        jsr     neg16           ; negate result
mulsi2_return:
        rts

;;; Unsigned division: dividend / divisor = quotient ... reminder
;;; @praram R1 dividend
;;; @praram R2 divisor
;;; @return R0 quotient
;;; @return R1 reminder
;;; @clobber R2
udiv16:
        lda     R2H
        ora     R2L
        beq     udiv16_return   ; branch if R2==0
        lda     #1              ; A=bits
        sta     R0L             ; save bits
        bne     udiv16_prep     ; always branch
udiv16_prep_loop:
        asl     R2L
        rol     R2H             ; divisor <<= 1
        inc     R0L             ; ++bits
udiv16_prep:
        lda     R2H
        bpl     udiv16_prep_loop ; while msb(divisor) == 0
        lda     R0L
        pha                     ; push bits
        lda     #0
        sta     R0H
        sta     R0L             ; quotient=0
        beq     udiv16_enter_loop ; always branch
udiv16_loop:
        pha                     ; save bits
        lsr     R2H
        ror     R2L             ; divisor >>= 1
        asl     R0L
        rol     R0H             ; quotient <<= 1
udiv16_enter_loop:
        lda     R1L
        sec
        sbc     R2L
        sta     R1L
        lda     R1H
        sbc     R2H
        sta     R1H             ; dividend -= divisor
        bcc     udiv16_readd
        inc     R0L             ; quotient |= 1
        bne     udiv16_next     ; always branch
udiv16_readd:
        lda     R1L
        clc
        adc     R2L
        sta     R1L
        lda     R1H
        adc     R2H
        sta     R1H             ; dividend += divisor
udiv16_next:
        pla
        sec
        sbc     #1              ; --bits
        bne     udiv16_loop     ; while bits != 0
udiv16_return:
        rts

;;; Division helper
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R0 quotient
;;; @return R1 reminder
;;; @clobber R2
divsi2:
        lda     R1H
        eor     R2H
        pha                     ; save hi(dividend^divisor)
        lda     R2H             ; A=hi(divisor)
        bpl     divsi2_dividend
        ldx     #R2
        jsr     neg16           ; negate divisor
divsi2_dividend:
        lda     R1H             ; A=hi(dividend)
        bpl     divsi2_divide
        ldx     #R1
        jsr     neg16           ; negate dividend
divsi2_divide:
        jsr     udiv16          ; R0=result
        pla                     ; A=(dividend^divisor)
        bpl     divsi2_return
        ldx     #R0L
        jsr     neg16           ; negate result
divsi2_return:
        rts
