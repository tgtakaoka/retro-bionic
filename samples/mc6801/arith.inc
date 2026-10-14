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

;;; Multiply; result = multiplicand * multiplier
;;; The low 16 bits of a product don't depend on the operands' signs. A
;;; high byte of 0 or $FF, a small positive or negative operand, needs
;;; no MUL for its cross product: $FF*n adds -n to the high byte.
;;; @param D multiplicand
;;; @param X multiplier
;;; @return D result
;;; @clobber R0 R1 R2
mul16:
        std     R1              ; R1=multiplicand
        beq     mul16_end       ; branch if multiplicand == 0
        stx     R2              ; R2=multiplier
        beq     mul16_zero      ; branch if multiplier == 0
        tsta
        bne     mul16_high      ; branch if high(multiplicand) != 0
        ldaa    R2H
        bne     mul16_multiplier ; branch if high(multiplier) != 0
        ldaa    R2L
        mul                     ; low(multiplicand)*low(multiplier)
mul16_end:
        rts
mul16_zero:
        ldd     #0
        rts
mul16_multiplier:
        inca
        bne     mul16_cross     ; branch if high(multiplier) != $FF
        ldaa    R2L
        mul                     ; low(multiplicand)*low(multiplier)
        suba    R1L             ; low(multiplicand)*$FF00
        rts
mul16_high:
        inca
        bne     mul16_cross     ; branch if high(multiplicand) != $FF
        ldaa    R2H
        beq     mul16_ff_low    ; branch if high(multiplier) == 0
        inca
        bne     mul16_cross     ; branch if high(multiplier) != $FF
        ldaa    R2L
        mul                     ; low(multiplicand)*low(multiplier)
        suba    R1L             ; low(multiplicand)*$FF00
        suba    R2L             ; $FF00*low(multiplier)
        rts
mul16_ff_low:
        ldaa    R2L
        mul                     ; low(multiplicand)*low(multiplier)
        suba    R2L             ; $FF00*low(multiplier)
        rts
mul16_cross:
        ldaa    R1H
        ldab    R2L
        mul                     ; high(multiplicand)*low(multiplier)
        stab    R0H
        ldaa    R1L
        ldab    R2H
        mul                     ; low(multiplicand)*high(multiplier)
        addb    R0H
        stab    R0H             ; R0H=sum of cross products
        ldaa    R1L
        ldab    R2L
        mul                     ; low(multiplicand)*low(multiplier)
        adda    R0H
        rts

;;; Square; result = value * value
;;; The square doesn't depend on the sign, and a magnitude below 256
;;; squares in one MUL.
;;; @param D value
;;; @return D result
;;; @clobber R0
sqr16:
        tsta
        bpl     sqr16_plus
        nega
        negb
        sbca    #0              ; negate value
sqr16_plus:
        bne     sqr16_wide      ; branch if |value| >= 256
        tba
        mul                     ; low(value)*low(value)
        rts
sqr16_wide:
        std     R0
        mul                     ; high(value)*low(value)
        lslb                    ; twice the cross product, its low byte
        stab    R0H             ; is all that reaches the result
        ldaa    R0L
        tab
        mul                     ; low(value)*low(value)
        adda    R0H
        rts

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram R1 dividend
;;; @praram R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber D X R2 sign
udiv16:
        clr     sign            ; positive quotient
        ldd     R1
        subd    R2
        bcs     udiv16_small    ; branch if dividend < divisor
        ldx     R2
        bsr     div16_unsigned
        std     R0              ; R0=quotient
        stx     R1              ; R1=remainder
        rts
udiv16_small:
        ldx     #0
        stx     R0              ; quotient=0, R1=remainder
        rts

;;; Division; dividend / divisor = quotient ... remainder
;;; @param D dividend
;;; @param X divisor
;;; @return D quotient
;;; @return X remainder
;;; @clobber R0 R1 R2
div16:
        staa    sign            ; save hi(dividend)
        bpl     div16_dividend
        nega
        negb
        sbca    #0              ; negate dividend
div16_dividend:
        std     R1              ; R1=|dividend|
        stx     R2
        bmi     div16_divisor   ; branch if divisor < 0
div16_compare:
        subd    R2
        bcs     div16_small     ; branch if |dividend| < |divisor|
;;; @param X divisor
;;; @param R1 dividend, not less than divisor
;;; @param R2 divisor
;;; @param sign sign of quotient at bit 7
div16_unsigned:
        dex
        cpx     #127
        bcc     div16_wide      ; branch if divisor == 0 or divisor >= 128
        ldd     R1              ; A=remainder
        cmpa    R2L
        bcc     div16_wide      ; branch if quotient >= 256
        jsr     udiv16_8_shift  ; the quotient fits in 8 bits
        tst     sign
        bmi     div16_byte_negate
        rts
div16_byte_negate:
        negb
        ldaa    #$FF            ; D=-quotient, quotient isn't 0
        rts
div16_small:
        ldx     R1              ; X=remainder
        ldd     #0              ; D=quotient
        rts
div16_divisor:
        com     sign            ; flip sign of quotient
        ldd     R2
        nega
        negb
        sbca    #0              ; negate divisor
        std     R2              ; R2=|divisor|
        ldx     R2
        ldd     R1
        jmp     div16_compare
div16_wide:
        ldd     R2
        beq     div16_small     ; divide by zero
        ldx     #1              ; X=quotient bits
div16_prep_loop:
        lsld                    ; divisor <<= 1
        bcs     div16_prep_end  ; until divisor overflows
        cmpa    R1H
        bcs     div16_prep_next
        bne     div16_prep_end  ; or divisor > dividend
        cmpb    R1L
        bhi     div16_prep_end
div16_prep_next:
        inx                     ; ++bits
        bra     div16_prep_loop
div16_prep_end:
        rora
        rorb                    ; divisor >>= 1, C back into msb
        std     R2              ; R2=divisor aligned to dividend
        ldd     #~1
        std     R0              ; R0=~quotient, the first bit 1
        ldd     R1
        subd    R2              ; the first subtraction always fits
        bra     div16_next
div16_loop:
        lsr     R2H
        ror     R2L             ; divisor >>= 1
        subd    R2              ; dividend -= divisor
        bcc     div16_bit       ; C=0
        addd    R2              ; dividend += divisor, C=1
div16_bit:
        rol     R0L
        rol     R0H             ; ~quotient = ~quotient<<1 | C
div16_next:
        dex                     ; --bits
        bne     div16_loop      ; while bits != 0
        std     R1
        ldx     R1              ; X=remainder
        ldd     R0
        tst     sign
        bmi     div16_negate
        coma
        comb                    ; D=quotient
        rts
div16_negate:
        addd    #1              ; D=-quotient=~quotient+1
        rts

;;; Division by a divisor from 1 to 127; dividend / divisor = quotient ... remainder
;;; @param D dividend
;;; @param X divisor, 1 to 127
;;; @return D quotient
;;; @return X |remainder|
;;; @clobber R1 R2 sign
div16_8:
        tsta
        bpl     udiv16_8        ; branch if dividend >= 0
        nega
        negb
        sbca    #0              ; negate dividend
        bsr     udiv16_8
        nega
        negb
        sbca    #0              ; negate quotient
        rts

;;; Unsigned division by a divisor from 1 to 127; dividend / divisor = quotient ... remainder
;;; A dividend below 256 has a quotient below 8 for a divisor of 32 or
;;; more, which subtracts faster than it shifts.
;;; @param D dividend
;;; @param X divisor, 1 to 127
;;; @return D quotient
;;; @return X remainder
;;; @clobber R1 R2 sign
udiv16_8:
        stx     R2              ; R2L=divisor
        tsta
        bne     udiv16_8_high   ; branch if dividend >= 256
        cmpb    R2L
        bcs     udiv16_8_zero   ; branch if dividend < divisor
        ldaa    R2L
        cmpa    #32
        bcs     udiv16_8_byte   ; branch if divisor < 32
        ldaa    #-1             ; A=quotient-1
udiv16_8_sub:
        inca                    ; ++quotient
        subb    R2L             ; dividend -= divisor
        bcc     udiv16_8_sub    ; at most 8 times as divisor >= 32
        addb    R2L             ; B=remainder
        ldx     #0
        abx                     ; X=remainder
        tab
        clra                    ; D=quotient
        rts
udiv16_8_zero:
        ldx     #0
        abx                     ; X=remainder
        clrb                    ; D=quotient
        rts
udiv16_8_high:
        cmpa    R2L
        bcs     udiv16_8_shift  ; branch if quotient < 256
        std     R1              ; R1=dividend
        clr     sign            ; positive quotient
        jmp     div16_wide
udiv16_8_byte:
        clra                    ; A=hi(dividend)
udiv16_8_shift:
;;; The quotient fits in 8 bits: shift the low byte of the dividend
;;; through A as the remainder, and the quotient into B behind it. A
;;; divisor below 128 keeps the shifted remainder within A.
        lsld                    ; remainder = remainder<<1 | msb(dividend)
        cmpa    R2L
        bcs     udiv16_8_1
        suba    R2L             ; remainder -= divisor
        incb                    ; quotient |= 1
udiv16_8_1:
        lsld                    ; remainder = remainder<<1 | msb(dividend)
        cmpa    R2L
        bcs     udiv16_8_2
        suba    R2L             ; remainder -= divisor
        incb                    ; quotient |= 1
udiv16_8_2:
        lsld                    ; remainder = remainder<<1 | msb(dividend)
        cmpa    R2L
        bcs     udiv16_8_3
        suba    R2L             ; remainder -= divisor
        incb                    ; quotient |= 1
udiv16_8_3:
        lsld                    ; remainder = remainder<<1 | msb(dividend)
        cmpa    R2L
        bcs     udiv16_8_4
        suba    R2L             ; remainder -= divisor
        incb                    ; quotient |= 1
udiv16_8_4:
        lsld                    ; remainder = remainder<<1 | msb(dividend)
        cmpa    R2L
        bcs     udiv16_8_5
        suba    R2L             ; remainder -= divisor
        incb                    ; quotient |= 1
udiv16_8_5:
        lsld                    ; remainder = remainder<<1 | msb(dividend)
        cmpa    R2L
        bcs     udiv16_8_6
        suba    R2L             ; remainder -= divisor
        incb                    ; quotient |= 1
udiv16_8_6:
        lsld                    ; remainder = remainder<<1 | msb(dividend)
        cmpa    R2L
        bcs     udiv16_8_7
        suba    R2L             ; remainder -= divisor
        incb                    ; quotient |= 1
udiv16_8_7:
        lsld                    ; remainder = remainder<<1 | msb(dividend)
        cmpa    R2L
        bcs     udiv16_8_8
        suba    R2L             ; remainder -= divisor
        incb                    ; quotient |= 1
udiv16_8_8:
        staa    R1L
        clra
        staa    R1H
        ldx     R1              ; X=remainder
        rts                     ; D=quotient
