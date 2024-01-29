;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6801

;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber A X R0 R1 R2
print_uint16:
        ldaa    R0H
        oraa    R0L
        beq     print_uint16_zero
print_uint16_loop:
        ldx     R0
        stx     R1                ; R1=value
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
        pshx                    ; push R1
        ldx     R2
        pshx                    ; push R2
        ldaa    R0H
        bpl     print_int16_print
        ldaa    #'-'
        jsr     putchar
        ldd     R0
        bsr     neg_D
        std     R0
print_int16_print:
        bsr     print_uint16
        pulx                    ; pop R2
        stx     R2
        pulx                    ; pop R1
        stx     R1
        rts

;;; Negate D
;;; @param D register
neg_D:
        coma
        comb
        addb    #1
        adca    #0
        rts

;;; Addition: R0 = R1 + R2
;;; @clobber D
add16:
        ldd     R1
        addd    R2
        std     R0
        rts

;;; Subtraction: R0 = R1 + R2
;;; @clobber D
sub16:
        ldd     R1
        subd    R2
        std     R0
        rts

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 result
;;; @clobber R0 R1 R2
umul16:
        ldaa    R1L
        ldab    R2L
        mul                     ; low(multiplicand)*low(multiplier)
        std     R0
        ldaa    R1L
        ldab    R2H
        mul                     ; low(multiplicand)*high(multiplier)
        addb    R0H
        stab    R0H
        ldaa    R1H
        ldab    R2L
        mul                     ; high(multiplicand)*low(multiplier)
        addb    R0H
        stab    R0H
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
        ldd     R2
        bpl     mul16_multiplicand
        jsr     neg_D           ; negate multiplier
mul16_multiplicand:
        std     R2
        ldd     R1
        bpl     mul16_multiply
        jsr     neg_D           ; negate multiplicand
mul16_multiply:
        std     R1
        bsr     umul16          ; R0=result
        pula                    ; A=(multiplicand^multiplier)
        tsta
        bpl     mul16_end
        ldd     R0
        jsr     neg_D           ; negate result
        std     R0
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
        ldx     #1              ; X=bits
        bra     udiv16_prep
udiv16_prep_loop:
        ldd     R2
        lsld
        std     R2              ; divisor <<= 1
        inx                     ; ++bits
udiv16_prep:
        ldaa    R2H
        bpl     udiv16_prep_loop ; while msb(divisor) == 0
        clra
        clrb
        std     R0              ; quotient=0
        bra     udiv16_enter_loop
udiv16_loop:
        ldd     R2
        lsrd
        std     R2              ; divisor >>= 1
        ldd     R0
        lsld
        std     R0              ; quotient <<= 1
udiv16_enter_loop:
        ldd     R1
        subd    R2
        std     R1              ; dividend -= divisor
        bcs     udiv16_readd
        inc     R0L             ; quotient |= 1
        bra     udiv16_next
udiv16_readd:
        addd    R2
        std     R1              ; dividend += divisor
udiv16_next:
        dex                     ; --bits
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
        ldd     R2
        bpl     div16_dividend
        jsr     neg_D           ; negate divisor
div16_dividend:
        std     R2
        ldd     R1
        bpl     div16_divide
        jsr     neg_D           ; negate dividend
div16_divide:
        std     R1
        jsr     udiv16          ; R0=quotient
        pula                    ; A=(dividend^divisor)
        tsta
        bpl     div16_end
        ldd     R0
        jsr     neg_D           ; negate result
        std     R0
div16_end:
        rts
