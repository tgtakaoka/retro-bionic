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

;;; Multiply: result = multiplicand * multiplier
;;; @param X multiplicand
;;; @param Y multiplier
;;; @return X result
;;; @clobber C Y
        longa   on
mul16:
        phy                     ; save multiplier
        txa
        eor     1,S
        sta     1,S             ; save (multiplicand^multiplier)
        tya
        bpl     mul16_multiplicand
        eor     #$FFFF
        inc     A
        tay                     ; negate multiplier
mul16_multiplicand:
        txa
        bpl     mul16_multiply
        eor     #$FFFF
        inc     A
        tax                     ; negate multiplicand
mul16_multiply:
        jsr     umul16          ; R0=result
        pla                     ; A=(multiplicand^multiplier)
        bpl     mul16_return
        txa
        eor     #$FFFF
        inc     A
        tax                     ; negate result
mul16_return:
        rts

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram X dividend
;;; @praram Y divisor
;;; @return X quotient
;;; @return Y remainder
;;; @clobber C
        longa   on
udiv16:
        tya                     ; C=divisor
        beq     udiv16_return   ; branch if Y==0
        ldy     #0              ; y=bits
udiv16_prep_loop:
        iny
        asl     A               ; divisor <<= 1
        bcc     udiv16_prep_loop
        sec
        ror     A               ; divisor >>= 1
        pha                     ; push divisor
        phx                     ; push dividend
        ldx     #0              ; X=quotient
        bra     udiv16_enter_loop
udiv16_loop:
        lda     3,S
        lsr     A
        sta     3,S             ; divisor >>= 1
        txa
        asl     A
        tax                     ; quotient <<= 1
udiv16_enter_loop:
        lda     1,S
        sec
        sbc     3,S             ; dividend -= divisor
        bcc     udiv16_next     ; if dividend < divisor
        sta     1,S
        inx                     ; quotient |= 1
udiv16_next:
        dey
        bne     udiv16_loop     ; while bits != 0
udiv16_return:
        ply                     ; remainder
        pla                     ; discard divisor
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
