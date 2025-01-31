;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6800

;;; Print signed 16-bit integer as decimal
;;; @param D value
;;; @clobber D X R0 R1 R2
print_int16:
        stab    R0L
        staa    R0H
        bpl     print_uint16
        ldaa    #'-'
        jsr     putchar
        ldaa    R0H
        nega
        negb
        sbca    #0
        staa    R0H
        stab    R0L
;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber A X R0 R1 R2
print_uint16:
        ldx     R0
        beq     print_uint16_zero
print_uint16_loop:
        stx     R1
        beq     print_uint16_end ; branch if value == 0
        ldx     #10
        stx     R2
        jsr     udiv16          ; R0=value/10
        ldab    R1L
        pshb                    ; push remainder
        ldx     R0
        bsr     print_uint16_loop
        pula
print_uint16_zero:
        oraa    #'0'
        jmp     putchar
print_uint16_end:
        rts   

;;; Signed compare R0=sign(R1-R2)
;;; @return R0=A=0;  BEQ (minuend == subtrahend)
;;;         R0=A=1;  BPL (minuend > subtrahend)
;;;         R0=A=-1; BMI (minuend < subtrahend)
;;; @clobber A
cmp16:
        ldaa    R1L
        suba    R2L
        staa    R0L
        ldaa    R1H
        sbca    R2H
        staa    R0H
        oraa    R0L
        beq     cmp16_end       ; return with R0=0
        ldaa    R0H
        eora    R1H
        staa    R0L             ; R0L=(minuend-subtrahend)^minuend
        ldaa    R1H
        eora    R2H             ; minuend^subtrahend
        anda    R0L             ; overflow flag
        eora    R0H
        bmi     cmp16_lt
cmp16_gt:
        clr     R0H
        ldaa    #1
        bra     cmp16_end
cmp16_lt:
        ldaa    #-1
        staa    R0H
cmp16_end:
        staa    R0L
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
        ldaa    R1L
        adda    R0L
        staa    R0L
        ldaa    R1H
        adca    R0H
        staa    R0H             ; result += multiplicand
umul16_next:
        lsl     R1L
        rol     R1H             ; multiplicand <<= 1
umul16_check:
        ldaa    R2H
        oraa    R2L
        bne     umul16_loop     ; while multiplier != 0
umul16_end:
        rts

;;; Multiply; result = multiplicand * multiplier
;;; @param D multiplicand
;;; @param X multiplier
;;; @return D result
;;; @return X result
;;; @clobber R0 R1 R2
mul16:
        staa    R1H
        stab    R1L
        stx     R2
        eora    R2H
        staa    sign            ; save hi(multiplicand^multiplier)
        ldab    R2L
        ldaa    R2H             ; A=hi(multiplier)
        bpl     mul16_multiplicand
        nega
        negb
        sbca    #0              ; negate multiplier
mul16_multiplicand:
        staa    R2H
        stab    R2L
        ldab    R1L
        ldaa    R1H             ; A=hi(multiplicand)
        bpl     mul16_multiply
        nega
        negb
        sbca    #0              ; negate multiplicand
mul16_multiply:
        staa    R1H
        stab    R1L
        bsr     umul16          ; R0=result
        ldaa    R0H
        ldab    R0L
        tst     sign            ; A=(multiplicand^multiplier)
        bpl     mul16_end
        nega
        negb
        sbca    #0               ; negate result
mul16_end:
        staa    R0H
        stab    R0L
        ldx     R0
        rts

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram R1 dividend
;;; @praram R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber R2
udiv16:
        ldaa    R2H
        oraa    R2L
        beq     udiv16_end      ; branch if R2==0
        ldaa    R2H             ; A:B=divisor
        ldab    R2L
        ldx     #1              ; X=bits
        bra     udiv16_prep
udiv16_prep_loop:
        lslb
        rola                    ; divisor <<= 1
        inx                     ; ++bits
udiv16_prep:
        tsta
        bpl     udiv16_prep_loop ; while msb(divisor) == 0
        staa    R2H              ; update divisore
        stab    R2L
        ldaa    R1H
        ldab    R1L             ; A:B=dividend
        clr     R0H
        clr     R0L             ; quotient=0
        bra     udiv16_enter_loop
udiv16_loop:
        lsr     R2H
        ror     R2L             ; divisor >>= 1
        lsl     R0L
        rol     R0H             ; quotient <<= 1
udiv16_enter_loop:
        subb    R2L
        sbca    R2H             ; dividend -= divisor
        bcs     udiv16_readd
        inc     R0L             ; quotient |= 1
        bra     udiv16_next
udiv16_readd:
        addb    R2L
        adca    R2H             ; dividend += divisor
udiv16_next:
        dex                     ; --bits
        bne     udiv16_loop     ; while bits != 0
udiv16_end:
        staa    R1H
        stab    R1L             ; R1=remainder
        rts

;;; Division; dividend / divisor = quotient ... remainder
;;; @param D dividend
;;; @param X divisor
;;; @return D quotient
;;; @return X quotient
;;; @clobber R0 R1 R2
div16:
        staa    R1H
        stab    R1L
        stx     R2
        eora    R2H
        staa    sign            ; save hi(dividend^divisor)
        ldab    R2L
        ldaa    R2H             ; A=hi(divisor)
        bpl     div16_dividend
        nega
        negb
        sbca    #0              ; negate divisor
        staa    R2H
        stab    R2L
div16_dividend:
        ldab    R1L
        ldaa    R1H             ; A=hi(dividend)
        bpl     div16_divide
        nega
        negb
        sbca    #0              ; negate dividend
        staa    R1H
        stab    R1L
div16_divide:
        bsr     udiv16          ; R0=quotient
        ldaa    R0H
        ldab    R0L
        tst     sign            ; A=(dividend^divisor)
        bpl     div16_end
        nega
        negb
        sbca    #0              ; negate result
div16_end:
        staa    R0H
        stab    R0L
        ldx     R0
        rts
