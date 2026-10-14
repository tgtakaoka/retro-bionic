;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     65816

;;; Print signed 16-bit integer as decimal
;;; @param X value
;;; @clobber C
        longa   off
print_int16:
        php
        rep     #P_M            ; 16-bit memory
        longa   on
        phy                     ; save Y
        phx                     ; save X
        txa
        bpl     print_int16_print
        lda     #'-'
        jsr     putchar
        txa
        eor     #$FFFF
        inc     A
        tax
print_int16_print:
        jsr     print_uint16
        plx                     ; restore X
        ply                     ; restore Y
        plp
        rts

;;; Print unsigned 16-bit integer as decimal
;;; @param X value
;;; @clobber C X Y
        longa   on
print_uint16:
        txa
        beq     print_uint16_digit ; branch if value==0
print_uint16_inner:
        txa
        beq     print_uint16_exit
        ldy     #10
        jsr     udiv16
        phy                     ; push remainder
        jsr     print_uint16_inner
        pla                     ; pop remainder
print_uint16_digit:
        ora     #'0'
        jmp     putchar
print_uint16_exit:
        rts

;;; Signed compare: minuend - subtrahend
;;; @param X minuend
;;; @param Y subtrahend
;;; @return C=0;  BEQ (minuend == subtrahend)
;;;         C=1;  BPL (minuend > subtrahend)
;;;         C=-1; BMI (minuend < subtrahend)
        longa   on
cmp16:
        txa                     ; C=minuend
        phy
        sec
        sbc     1,S             ; C=minuend-subtrahend
        beq     cmp16_eq        ; C=0
        bvs     cmp16_ov        ; branch if V=1
        bmi     cmp16_lt        ; branch if V=0, N=1
cmp16_gt:
        ply
        lda     #1              ; N^V=0
        rts
cmp16_ov:
        bmi     cmp16_gt        ; branch if V=1, N=1
cmp16_lt:
        ply
        lda     #-1             ; N^V=1
        rts
cmp16_eq:
        ply
        lda     #0
        rts

;;; Square: result = value * value
;;; @param X value
;;; @return X result
;;; @clobber C Y arith_work
        longa   on
sqr16:
        txa
        bpl     sqr16_abs       ; branch if value >= 0
        eor     #$FFFF
        inc     A
        tax                     ; |value|
sqr16_abs:
        tay                     ; multiplier=multiplicand=|value|
                                ; fall through to umul8

;;; Unsigned multiplication by a multiplier below 256
;;; @param C multiplicand, also in X
;;; @param Y multiplier, below 256
;;; @return C,X result
;;; @clobber Y arith_work
;;; A multiplicand below 256 takes an unrolled 8-bit by 8-bit
;;; multiply; others go to umul16 with X and Y, which clobbers Y.
        longa   on
umul8:
        beq     umul8_zero      ; branch if multiplicand == 0
        cmp     #$100
        bcs     umul16          ; branch if multiplicand >= 256
        xba
        dec     A
        sta     arith_work      ; multiplicand<<8 - 1, as ADC adds C=1
        tya
        lsr     A               ; C=lsb(multiplier)
        bcc     umul8_1
        adc     arith_work      ; result += multiplicand<<8
umul8_1:
        ror     A               ; result >>= 1, next multiplier bit to C
        bcc     umul8_2
        adc     arith_work      ; result += multiplicand<<8
umul8_2:
        ror     A               ; result >>= 1, next multiplier bit to C
        bcc     umul8_3
        adc     arith_work      ; result += multiplicand<<8
umul8_3:
        ror     A               ; result >>= 1, next multiplier bit to C
        bcc     umul8_4
        adc     arith_work      ; result += multiplicand<<8
umul8_4:
        ror     A               ; result >>= 1, next multiplier bit to C
        bcc     umul8_5
        adc     arith_work      ; result += multiplicand<<8
umul8_5:
        ror     A               ; result >>= 1, next multiplier bit to C
        bcc     umul8_6
        adc     arith_work      ; result += multiplicand<<8
umul8_6:
        ror     A               ; result >>= 1, next multiplier bit to C
        bcc     umul8_7
        adc     arith_work      ; result += multiplicand<<8
umul8_7:
        ror     A               ; result >>= 1, next multiplier bit to C
        bcc     umul8_8
        adc     arith_work      ; result += multiplicand<<8
umul8_8:
        ror     A               ; result >>= 1, next multiplier bit to C
umul8_zero:
        tax                     ; X=result
        rts

;;; Multiply: result = multiplicand * multiplier
;;; @param X multiplicand
;;; @param Y multiplier
;;; @return X result
;;; @clobber C Y arith_work
;;; A 16-bit product is the same signed or unsigned; a negative
;;; multiplier is negated only to shorten the loop. Magnitudes below
;;; 256 take umul8.
        longa   on
mul16:
        tya
        beq     umul8_zero      ; branch if multiplier == 0
        bpl     mul16_small     ; branch if multiplier > 0
        eor     #$FFFF
        inc     A
        tay                     ; negate multiplier
        txa
        eor     #$FFFF
        inc     A
        tax                     ; negate multiplicand
mul16_small:
        cpy     #$100
        bcs     umul16          ; branch if multiplier >= 256
        txa
        bpl     umul8           ; branch if multiplicand >= 0
        eor     #$FFFF
        inc     A               ; |multiplicand|
        cmp     #$100
        bcs     umul16          ; branch if |multiplicand| >= 256
        jsr     umul8           ; never reaches umul16 with the signed X
        eor     #$FFFF
        inc     A
        tax                     ; negate result
        rts

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param X multiplicand
;;; @param Y multiplier
;;; @return X result
;;; @clobber C Y
        longa   on
umul16:
        lda     #0
        pha                     ; result (1,S)
        bra     umul16_check
umul16_loop:
        tya
        lsr     A               ; multiplier >>= 1
        tay
        bcc     umul16_sr
        txa
        clc
        adc     1,S
        sta     1,S             ; result += multiplicand
umul16_sr:
        txa
        asl     A               ; multiplicand <<= 1
        tax
umul16_check:
        tya
        bne     umul16_loop     ; while multiplier != 0
umul16_end:
        plx                     ; X=result
        rts

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram X dividend
;;; @praram Y divisor
;;; @return X quotient
;;; @return Y remainder
;;; @clobber C arith_work
;;; A divisor below 128 and a dividend below divisor<<8 take a long
;;; division keeping the remainder in the upper byte of C and shifting
;;; quotient bits into the lower byte.
        longa   on
udiv16:
        stx     arith_work+2    ; dividend
        tya                     ; C=divisor
        beq     udiv16_zero     ; branch if Y==0
        cmp     #$80
        bcs     udiv16_wide     ; branch if divisor >= 128
        xba
        dec     A
        sta     arith_work      ; divisor<<8 - 1
        txa                     ; C=dividend
        cmp     arith_work
        bcs     udiv16_wide     ; branch if quotient may be >= 256
udiv16_s1:
        asl     A
        cmp     arith_work
        bcc     udiv16_s2
        sbc     arith_work      ; remainder -= divisor, quotient |= 1
udiv16_s2:
        asl     A
        cmp     arith_work
        bcc     udiv16_s3
        sbc     arith_work      ; remainder -= divisor, quotient |= 1
udiv16_s3:
        asl     A
        cmp     arith_work
        bcc     udiv16_s4
        sbc     arith_work      ; remainder -= divisor, quotient |= 1
udiv16_s4:
        asl     A
        cmp     arith_work
        bcc     udiv16_s5
        sbc     arith_work      ; remainder -= divisor, quotient |= 1
udiv16_s5:
        asl     A
        cmp     arith_work
        bcc     udiv16_s6
        sbc     arith_work      ; remainder -= divisor, quotient |= 1
udiv16_s6:
        asl     A
        cmp     arith_work
        bcc     udiv16_s7
        sbc     arith_work      ; remainder -= divisor, quotient |= 1
udiv16_s7:
        asl     A
        cmp     arith_work
        bcc     udiv16_s8
        sbc     arith_work      ; remainder -= divisor, quotient |= 1
udiv16_s8:
        asl     A
        cmp     arith_work
        bcc     udiv16_s9
        sbc     arith_work      ; remainder -= divisor, quotient |= 1
udiv16_s9:
        tay                     ; Y=remainder:quotient
        and     #$FF
        tax                     ; X=quotient
        tya
        xba
        and     #$FF
        tay                     ; Y=remainder
        rts
udiv16_wide:
        ldx     #1              ; X=bits
        tya                     ; C=divisor
        bmi     udiv16_prep_end ; branch if msb(divisor) == 1
udiv16_prep:
        cmp     arith_work+2
        bcs     udiv16_prep_end ; until divisor >= dividend
        inx                     ; ++bits
        asl     A               ; divisor <<= 1
        bpl     udiv16_prep     ; while msb(divisor) == 0
udiv16_prep_end:
        sta     arith_work      ; divisor
        lda     arith_work+2    ; C=dividend
        stz     arith_work+2    ; quotient=0
        bra     udiv16_enter_loop
udiv16_loop:
        lsr     arith_work      ; divisor >>= 1
udiv16_enter_loop:
        cmp     arith_work
        bcc     udiv16_next     ; if dividend < divisor
        sbc     arith_work      ; dividend -= divisor
udiv16_next:
        rol     arith_work+2    ; quotient = quotient<<1 | C
        dex
        bne     udiv16_loop     ; while bits != 0
        tay                     ; Y=remainder
udiv16_zero:
        ldx     arith_work+2    ; X=quotient
        rts

;;; Division: dividend / divisor = quotient ... remainder
;;; @param X dividend
;;; @param Y divisor
;;; @return X quotient
;;; @return Y remainder
;;; @clobber C
        longa   on
div16:
        phy                     ; save divisor
        txa
        eor     1,S
        sta     1,S             ; save (dividend^divisor)
        tya
        bpl     div16_dividend
        eor     #$FFFF
        inc     A
        tay                     ; negate divisor
div16_dividend:
        txa
        bpl     div16_multiply
        eor     #$FFFF
        inc     A
        tax                     ; negate dividend
div16_multiply:
        jsr     udiv16          ; R0=result
        pla                     ; A=(dividend^divisor)
        bpl     div16_return
        txa
        eor     #$FFFF
        inc     A
        tax                     ; negate result
div16_return:
        rts
