;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6811

;;; Print signed 16-bit integer as decimal
;;; @param D value
;;; @clobber D X
print_int16:
        tsta
        bpl     print_uint16
        psha
        ldaa    #'-'
        jsr     putchar
        pula
        nega
        negb
        sbca    #0        ; negate value
;;; Print unsigned 16-bit integer as decimal
;;; @param D value
;;; @clobber D X
print_uint16:
        addd    #0
        beq     print_uint16_zero
print_uint16_loop:
        addd    #0
        beq     print_uint16_end ; branch if value == 0
        ldx     #10
        idiv                    ; D/X, D=remainder, X=quotient
        pshb                    ; push remainder
        xgdx                    ; D=quotient
        bsr     print_uint16_loop
        pula
print_uint16_zero:
        oraa    #'0'
        jmp     putchar
print_uint16_end:
        rts

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param D multiplicand
;;; @param X multiplier
;;; @return D result
;;; @clobber R0 R1 R2
umul16:
        std     R1
        stx     R2
        ldaa    R2L
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
        ldd     R0
        rts

;;; Multiply; result = multiplicand * multiplier
;;; @param D multiplicand
;;; @param X multiplier
;;; @return D result
;;; @clobber R0 R1 R2
mul16:
        std     R1
        stx     R2
        eora    R2H
        staa    sign            ; save hi(multiplicand^multiplier)
        ldaa    R1H
        bpl     mul16_multiplier
        nega
        negb
        sbca    #0              ; negate multiplicand
mul16_multiplier:
        xgdx
        tsta
        bpl     mul16_multiply
        nega
        negb
        sbca    #0              ; negate multiplier
mul16_multiply:
        xgdx
        bsr     umul16          ; R0=result
        tst     sign            ; (multiplicand^multiplier)
        bpl     mul16_end
        nega
        negb
        sbca    #0              ; negate result
mul16_end:
        rts

;;; Division; dividend / divisor = quotient ... remainder
;;; @param D dividend
;;; @param X divisor
;;; @return D quotient
;;; @return X remainder
;;; @clobber R0 R1 R2
div16:
        std     R1
        stx     R2
        eora    R2H
        staa    sign            ; save hi(dividend^divisor)
        ldaa    R1H
        bpl     div16_divisor
        nega
        negb
        sbca    #0              ; negate dividend
div16_divisor:
        xgdx
        tsta
        bpl     div16_divide
        nega
        negb
        sbca    #0              ; negate divisor
div16_divide:
        xgdx
        idiv                    ; D/X, X=quotient, D=remainder
        xgdx
        tst     sign            ; A=(dividend^divisor)
        bpl     div16_end
        nega
        negb
        sbca    #0              ; negate result
div16_end:
        rts
