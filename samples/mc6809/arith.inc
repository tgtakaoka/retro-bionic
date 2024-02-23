;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6809

;;; Print signed 16-bit integer as decimal
;;; @param R0 value
;;; @clobber A X R0 R1 R2
print_int16:
        lda     R0H
        bpl     print_uint16
        lda     #'-'
        jsr     putchar
        ldd     R0
        bsr     neg_D
        std     R0
;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber A X R0 R1 R2
print_uint16:
        lda     R0H
        ora     R0L
        beq     print_uint16_zero
print_uint16_loop:
        ldx     R0
        stx     R1               ; R1=value
        beq     print_uint16_end ; branch if value == 0
        ldx     #10
        stx     R2              ; R2=10
        jsr     udiv16          ; R0=value/10
        lda     R1L
        pshs    A               ; push reminder
        bsr     print_uint16_loop
        puls    A
print_uint16_zero:
        ora     #'0'
        jmp     putchar
print_uint16_end:
        rts

;;; Negate D
;;; @param D register
neg_D:
        coma
        comb
        addb    #1
        adca    #0
        rts

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 result
;;; @clobber R0 R1 R2
umul16:
        lda     R1L
        ldb     R2L
        mul                     ; low(multiplicand)*low(multiplier)
        std     R0
        lda     R1L
        ldb     R2H
        mul                     ; low(multiplicand)*high(multiplier)
        addb    R0H
        stb     R0H
        lda     R1H
        ldb     R2L
        mul                     ; high(multiplicand)*low(multiplier)
        addb    R0H
        stb     R0H
        rts

;;; Multiply; result = multiplicand * multiplier
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 result
;;; @clobber R1 R2
mul16:
        lda     R1H
        eora    R2H
        pshs    A               ; save hi(multiplicand^multiplier)
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
        lda     ,S+             ; A=(multiplicand^multiplier)
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
        ldd     R2
        beq     udiv16_end      ; divide by zero
        ldx     #1              ; X=bits
        bra     udiv16_prep
udiv16_prep_loop:
        lslb
        rola                    ; divisor <<= 1
        leax    1,X             ; ++bits
udiv16_prep:
        tsta
        bpl     udiv16_prep_loop ; while msb(divisor) == 0
        std     R2
        clra
        clrb
        std     R0              ; quotient=0
        bra     udiv16_enter_loop
udiv16_loop:
        ldd     R2
        lsra
        rorb
        std     R2              ; divisor >>= 1
        ldd     R0
        lslb
        rola
        std     R0              ; quotient <<= 1
udiv16_enter_loop:
        ldd     R1
        subd    R2              ; D = dividend - divisor
        blo     udiv16_readd
        inc     R0L             ; quotient |= 1
        bra     udiv16_next
udiv16_readd:
        addd    R2              ; D=dividend + divisor
udiv16_next:
        std     R1
        leax    -1,X            ; --bits
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
        lda     R1H
        eora    R2H
        pshs    A               ; save hi(dividend^divisor)
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
        lda     ,S+             ; A=(dividend^divisor)
        bpl     div16_end
        ldd     R0
        jsr     neg_D           ; negate result
        std     R0
div16_end:
        rts
