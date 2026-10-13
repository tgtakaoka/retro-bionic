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
        coma
        comb
        addb    #1
        adca    #0
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
;;; @return D result
;;; @clobber R1 R2
umul16:
        clra
        clrb                    ; result=0
        tst     R2H
        beq     umul16_low      ; if hi(multiplier) == 0
        ldab    R1L             ; B=lo(multiplicand)
umul16_high:
        lsr     R2H             ; hi(multiplier) >>= 1
        beq     umul16_high_last ; if hi(multiplier) == 0
        bcc     umul16_high_next ; if lsb(multiplier) == 0
        aba                     ; hi(result) += B
umul16_high_next:
        lslb                    ; B <<= 1
        bra     umul16_high
umul16_high_last:
        aba                     ; hi(result) += B
        clrb
umul16_low:
        lsr     R2L             ; lo(multiplier) >>= 1
        beq     umul16_last     ; if lo(multiplier) == 0
        bcc     umul16_next     ; if lsb(multiplier) == 0
        addb    R1L
        adca    R1H             ; result += multiplicand
umul16_next:
        lsl     R1L
        rol     R1H             ; multiplicand <<= 1
        bra     umul16_low
umul16_last:
        bcc     umul16_end      ; if lsb(multiplier) == 0
        addb    R1L
        adca    R1H             ; result += multiplicand
umul16_end:
        staa    R0H
        stab    R0L
        rts

;;; Multiply; result = multiplicand * multiplier
;;; @param D multiplicand
;;; @param X multiplier
;;; @return D result
;;; @return X result
;;; @clobber R0 R1 R2
mul16:
        stx     R2
        beq     mul16_zero      ; branch if multiplier == 0
        staa    R1H
        stab    R1L
        ldx     R1
        beq     mul16_zero      ; branch if multiplicand == 0
        eora    R2H
        staa    sign            ; save hi(multiplicand^multiplier)
        ldaa    R2H             ; A=hi(multiplier)
        bpl     mul16_multiplicand
        ldab    R2L
        coma
        comb
        addb    #1
        adca    #0              ; negate multiplier
        staa    R2H
        stab    R2L
mul16_multiplicand:
        ldaa    R1H             ; A=hi(multiplicand)
        bpl     mul16_multiply
        ldab    R1L
        coma
        comb
        addb    #1
        adca    #0              ; negate multiplicand
        staa    R1H
        stab    R1L
mul16_multiply:
        bsr     umul16          ; D=R0=result
        tst     sign            ; A=(multiplicand^multiplier)
        bpl     mul16_end
        coma
        comb
        addb    #1
        adca    #0              ; negate result
        staa    R0H
        stab    R0L
mul16_end:
        ldx     R0
        rts
mul16_zero:
        clra
        clrb
        stx     R0              ; X=R0=0
        rts

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber R2
udiv16:
        ldaa    R2H
        oraa    R2L
        beq     udiv16_end      ; branch if R2==0
        ldaa    R2H             ; A:B=divisor
        ldab    R2L
        cmpa    R1H
        bhi     udiv16_zero     ; branch if divisor > dividend
        bcs     udiv16_start
        cmpb    R1L
        bhi     udiv16_zero
udiv16_start:
        ldx     #1              ; X=bits
        bra     udiv16_prep_msb
udiv16_prep_loop:
        lslb
        rola                    ; divisor <<= 1
        inx                     ; ++bits
udiv16_prep:
        cmpa    R1H
        bhi     udiv16_prep_end ; until divisor > dividend
        bcs     udiv16_prep_msb
        cmpb    R1L
        bhi     udiv16_prep_end
udiv16_prep_msb:
        tsta
        bpl     udiv16_prep_loop ; while msb(divisor) == 0
udiv16_prep_end:
        staa    R2H              ; update divisor
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
udiv16_zero:
        ldx     #0
        stx     R0              ; quotient=0, R1=remainder
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
        ldaa    R2H             ; A=hi(divisor)
        bpl     div16_dividend
        ldab    R2L
        coma
        comb
        addb    #1
        adca    #0              ; negate divisor
        staa    R2H
        stab    R2L
div16_dividend:
        ldaa    R1H             ; A=hi(dividend)
        bpl     div16_divide
        ldab    R1L
        coma
        comb
        addb    #1
        adca    #0              ; negate dividend
        staa    R1H
        stab    R1L
div16_divide:
        jsr     udiv16          ; R0=quotient
        ldaa    R0H
        ldab    R0L
        tst     sign            ; A=(dividend^divisor)
        bpl     div16_end
        coma
        comb
        addb    #1
        adca    #0              ; negate result
        staa    R0H
        stab    R0L
div16_end:
        ldx     R0
        rts
