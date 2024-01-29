;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6800

;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber A X R0 R1 R2
print_uint16:
        ldaa    R0H
        oraa    R0L
        beq     print_uint16_zero
print_uint16_loop:
        ldx     R0
        stx     R1               ; R1=value
        beq     print_uint16_end ; branch if value == 0
        ldx     #10
        stx     R2              ; R2=10
        jsr     udiv16          ; R0=value/10
        ldaa    R1L
        psha                    ; push reminder
        bsr     print_uint16_loop
        pula
print_uint16_zero:
        oraa    #'0'
        jmp     putchar
print_uint16_end:
        rts   

;;; Print signed 16-bit integer as decimal
;;; @param R0 value
;;; @clobber A X R0
print_int16:
        ldx     R1
        bsr     push_X          ; push R1
        ldx     R2
        bsr     push_X          ; push R2
        ldaa    R0H
        bpl     print_int16_print
        ldaa    #'-'
        jsr     putchar
        ldx     R0
        bsr     neg_X
        stx     R0
print_int16_print:
        bsr     print_uint16
        bsr     pop_X           ; pop R2
        stx     R2
        bsr     pop_X           ; pop R1
        stx     R1
        rts

;;; Push X register
push_X:
        des
        des                     ; for return address
        psha
        tpa
        psha                    ; save CC
        stx     arith_work      ; save X
        tsx                     ; X=SP+1
        ldaa    4,X
        staa    2,X
        ldaa    5,X
        staa    3,X             ; move return address
        ldaa    arith_work      ; high(X)
        staa    4,X
        ldaa    arith_work+1    ; low(X)
        staa    5,X
        ldx     arith_work      ; restore X
        pula                    ; restore CC X[0]
        tap
        pula                    ; restore A  X[1]
        rts                     ;            X[2:3]

;;; Pop X register
pop_X:
        psha                    ; save A
        tpa
        psha                    ; save CC
        tsx                     ; X=SP+1
        ldx     4,X             ; pop X
        stx     arith_work
        tsx                     ; X=SP+1
        ldaa    2,X
        staa    4,X
        ldaa    3,X
        staa    5,X
        ldx     arith_work
        pula                    ; restore CC
        tap
        pula                    ; restore A
        ins
        ins                     ; adjust stack
        rts

;;; Negate D
;;; @param D register
neg_D:
        coma
        comb
        addb    #1
        adca    #0
        rts

;;; Negate RX
;;; @param X register
neg_X:
        stx     arith_work
        com     arith_work
        com     arith_work+1
        ldx     arith_work
        inx
        rts

;;; Addition: R0 = R1 + R2
;;; @clobber A
add16:
        ldaa    R1L
        adda    R2L
        staa    R0L
        ldaa    R1H
        adca    R2H
        staa    R0H
        rts

;;; Subtraction: R0 = R1 + R2
;;; @clobber A
sub16:
        ldaa    R1L
        suba    R2L
        staa    R0L
        ldaa    R1H
        sbca    R2H
        staa    R0H
        rts

;;; Signed compare R0=sign(R1-R2)
;;; @return R0=A=0;  BEQ (minuend == subtrahend)
;;;         R0=A=1;  BPL (minuend > subtrahend)
;;;         R0=A=-1; BMI (minuend < subtrahend)
;;; @clobber A
cmp16:
        bsr     sub16
        ldaa    R0H
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
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 result
;;; @clobber R1 R2
mul16:
        ldaa    R1H
        eora    R2H
        psha                    ; save hi(multiplicand^multiplier)
        ldab    R2L
        ldaa    R2H             ; A=hi(multiplier)
        bpl     mul16_multiplicand
        jsr     neg_D           ; negate multiplier
mul16_multiplicand:
        staa    R2H
        stab    R2L
        ldab    R1L
        ldaa    R1H             ; A=hi(multiplicand)
        bpl     mul16_multiply
        jsr     neg_D           ; negate multiplicand
mul16_multiply:
        staa    R1H
        stab    R1L
        bsr     umul16          ; R0=result
        pula                    ; A=(multiplicand^multiplier)
        tsta
        bpl     mul16_end
        ldaa    R0H
        ldab    R0L
        jsr     neg_D           ; negate result
        staa    R0H
        stab    R0L
mul16_end:
        rts

;;; Unsigned division: dividend / divisor = quotient ... reminder
;;; @praram R1 dividend
;;; @praram R2 divisor
;;; @return R0 quotient
;;; @return R1 reminder
;;; @clobber R2
udiv16:
        ldaa    R2H
        oraa    R2L
        beq     udiv16_end      ; branch if R2==0
        ldab    #1              ; B=bits
        bra     udiv16_prep
udiv16_prep_loop:
        lsl     R2L
        rol     R2H             ; divisor <<= 1
        incb                    ; ++bits
udiv16_prep:
        ldaa    R2H
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
        ldaa    R1L
        suba    R2L
        staa    R1L
        ldaa    R1H
        sbca    R2H
        staa    R1H             ; dividend -= divisor
        bcs     udiv16_readd
        inc     R0L             ; quotient |= 1
        bra     udiv16_next
udiv16_readd:
        ldaa    R1L
        adda    R2L
        staa    R1L
        ldaa    R1H
        adca    R2H
        staa    R1H             ; dividend += divisor
udiv16_next:
        decb                    ; --bits
        bne     udiv16_loop     ; while bits != 0
udiv16_end:
        rts

;;; Division; dividend / divisor = quotient ... reminder
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R0 quotient
;;; @return R1 reminder
;;; @clobber R2
div16:
        ldaa    R1H
        eora    R2H
        psha                    ; save hi(dividend^divisor)
        ldab    R2L
        ldaa    R2H             ; A=hi(divisor)
        bpl     div16_dividend
        jsr     neg_D           ; negate divisor
div16_dividend:
        staa    R2H
        stab    R2L
        ldab    R1L
        ldaa    R1H             ; A=hi(dividend)
        bpl     div16_divide
        jsr     neg_D           ; negate dividend
div16_divide:
        staa    R1H
        stab    R1L
        jsr     udiv16          ; R0=quotient
        pula                    ; A=(dividend^divisor)
        tsta
        bpl     div16_end
        ldaa    R0H
        ldab    R0L
        jsr     neg_D           ; negate result
        staa    R0H
        stab    R0L
div16_end:
        rts
