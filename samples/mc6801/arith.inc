;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6801

;;; Print signed 16-bit integer as decimal
;;; @param D value
;;; @clobber D X R0 R1 R2
print_int16:
        std     R0
        bpl     print_uint16
        psha
        ldaa    #'-'
        jsr     putchar
        pula
        coma
        comb
        addd    #1        ; negate value
        std     R0
;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber D X
print_uint16:
        ldd     R0
        beq     print_uint16_zero
print_uint16_loop:
        std     R1
        beq     print_uint16_end ; branch if value == 0
        ldx     #10
        stx     R2
        jsr     udiv16          ; R0=value/10
        ldab    R1L
        pshb                    ; push remainder
        ldd     R0
        bsr     print_uint16_loop
        pula
print_uint16_zero:
        oraa    #'0'
        jmp     putchar
print_uint16_end:
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
;;; @param D multiplicand
;;; @param X multiplier
;;; @return D result
;;; @clobber R0 R1 R2
mul16:
        std     R1
        stx     R2
        eora    R2H
        staa    sign            ; save hi(multiplicand^multiplier)
        ldd     R2
        bpl     mul16_multiplicand
        coma
        comb
        addd    #1              ; negate multiplier
        std     R2
mul16_multiplicand:
        ldd     R1
        bpl     mul16_multiply
        coma
        comb
        addd    #1              ; negate multiplicand
        std     R1
mul16_multiply:
        bsr     umul16          ; R0=result
        ldd     R0
        tst     sign            ; A=(multiplicand^multiplier)
        bpl     mul16_end
        coma
        comb
        addd    #1              ; negate result
mul16_end:
        rts

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram R1 dividend
;;; @praram R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber R2
udiv16:
        ldd     R2              ; D=divisor
        beq     udiv16_end      ; branch if divisor==0
        ldx     #1              ; X=bits
        bra     udiv16_prep
udiv16_prep_loop:
        lsld                    ; divisor <<= 1
        inx                     ; ++bits
udiv16_prep:
        tsta
        bpl     udiv16_prep_loop ; while msb(divisor) == 0
        std     R2               ; update divisor
        ldd     R1               ; D=dividend
        clr     R0H
        clr     R0L             ; quotient=0
        bra     udiv16_enter_loop
udiv16_loop:
        lsr     R2H
        ror     R2L             ; divisor >>= 1
        lsl     R0L
        rol     R0H             ; quotient <<= 1
udiv16_enter_loop:
        subd    R2              ; dividend -= divisor
        bcs     udiv16_readd
        inc     R0L             ; quotient |= 1
        bra     udiv16_next
udiv16_readd:
        addd    R2              ; dividend += divisor
udiv16_next:
        dex                     ; --bits
        bne     udiv16_loop     ; while bits != 0
udiv16_end:
        std     R1              ; R1=remainder
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
        ldd     R2
        bpl     div16_dividend
        coma
        comb
        addd    #1              ; negate divisor
        std     R2
div16_dividend:
        ldd     R1
        bpl     div16_divide
        coma
        comb
        addd    #1              ; negate dividend
        std     R1
div16_divide:
        bsr     udiv16          ; R0=quotient
        ldd     R0
        ldx     R1
        tst     sign            ; A=(dividend^divisor)
        bpl     div16_end
        coma
        comb
        addd    #1              ; negate result
div16_end:
        rts
