;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6811

;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber D X R0
print_uint16:
        ldd     R0
        beq     print_uint16_zero
print_uint16_loop:
        ldd     R0
        beq     print_uint16_end ; branch if value == 0
        ldx     #10
        idiv
        stx     R0              ; quotient
        pshb                    ; push reminder
        bsr     print_uint16_loop
        pula
print_uint16_zero:
        oraa    #'0'
        jmp     putchar
print_uint16_end:
        rts

;;; Print signed 16-bit integer as decimal
;;; @param R0 value
;;; @clobber D X R0
print_int16:
        ldaa    R0H
        bpl     print_int16_print
        ldaa    #'-'
        jsr     putchar
        ldd     R0
        bsr     neg_D
        std     R0
print_int16_print:
        bsr     print_uint16
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
;;; @param X multiplicand
;;; @param Y multiplier
;;; @return R0 result
;;; @clobber R1 R2
umul16:
        stx     R1
        sty     R2
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
        xgdx
        ldd     R1
        bpl     mul16_multiply
        jsr     neg_D           ; negate multiplicand
mul16_multiply:
        xgdy
        bsr     umul16          ; R0=result
        pula                    ; A=(multiplicand^multiplier)
        tsta
        bpl     mul16_end
        ldd     R0
        jsr     neg_D           ; negate result
        std     R0
mul16_end:
        rts

;;; Division; dividend / divisor = quotient ... reminder
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R0 quotient
;;; @return R1 reminder
div16:
        ldaa    R1H
        eora    R2H
        psha                    ; save hi(dividend^divisor)
        ldd     R2
        bpl     div16_dividend
        jsr     neg_D           ; negate divisor
div16_dividend:
        xgdx
        ldd     R1
        bpl     div16_divide
        jsr     neg_D           ; negate dividend
div16_divide:
        idiv                    ; D/X, X=quotient, D=reminder
        pula                    ; A=(dividend^divisor)
        tsta
        xgdx
        bpl     div16_end
        jsr     neg_D           ; negate result
div16_end:
        std     R0
        rts
