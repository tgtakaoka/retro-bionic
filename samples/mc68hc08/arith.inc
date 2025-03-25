;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber A X R0 R1 R2
print_uint16:
        ldhx    #print_uint16_buf
print_uint16_loop:
        pshx
        pshh
        ldhx    R0
        sthx    R1              ; dividend = value
        beq     print_uint16_zero ; branch if value == 0
        ldhx    #10
        sthx    R2
        jsr     udiv16
        lda     R1L             ; remainder
        ora     #'0'
        pulh
        pulx
        sta     ,x
        aix     #1
        bra     print_uint16_loop
print_uint16_zero:
        pulh
        pulx
        cphx    #print_uint16_buf
        bne     print_uint16_print
        lda     #'0'
        jmp     putchar
print_uint16_print:
        aix     #-1
        lda     ,x
        jsr     putchar
        cphx    #print_uint16_buf
        bne     print_uint16_print
        rts
print_uint16_buf:
        rmb     8

;;; Print signed 16-bit integer as decimal
;;; @param R0 value
;;; @clobber A X R0
print_int16:
        ldhx    R1
        pshx
        pshh                    ; push R1
        ldhx    R2
        pshx
        pshh                    ; push R2
        lda     R0H
        bpl     print_int16_print
        lda     #'-'
        jsr     putchar
        lda     R0H
        nega
        neg     R0L
        sbc     #0
        sta     R0H             ; negate R0
print_int16_print:
        bsr     print_uint16
        pulh
        pulx
        sthx    R2              ; pop R2
        pulh
        pulx
        sthx    R1              ; pop R1
        rts

;;; Addition: R0 = R1 + R2
;;; @return HX addition
;;; @clobber A
add16:
        lda     R1L
        add     R2L
        sta     R0L
        lda     R1H
        adc     R2H
        sta     R0H
        ldhx    R0
        rts

;;; Subtraction: R0 = R1 + R2
;;; @return HX subtraction
;;; @clobber A
sub16:
        lda     R1L
        sub     R2L
        sta     R0L
        lda     R1H
        sbc     R2H
        sta     R0H
        ldhx    R0
        rts

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 result
umul16:
        lda     R2H             ; A=high(multiplier)
        ldx     R1L             ; X=low(multiplicand)
        mul                     ; X:A=low(multiplicand)*high(multiplier)
        sta     R0H             ; high(result)=A
        lda     R2L             ; A=low(multiplier)
        ldx     R1H             ; X=high(multiplicand)
        mul                     ; X:A=high(multiplicand)*low(multiplier)
        add     R0H
        sta     R0H             ; high(result)+=A
        lda     R2L             ; A=low(multiplier)
        ldx     R1L             ; X=low(multiplicand)
        mul                     ; X:A=result
        sta     R0L             ; low(result)=A
        txa
        add     R0H
        sta     R0H             ; high(result)+=X
        rts

;;; Multiply; result = multiplicand * multiplier
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R0 result
;;; @return HX result
;;; @clobber R1 R2
mul16:
        lda     R1H
        eor     R2H
        psha                    ; save hi(multiplicand^multiplier)
        lda     R2H             ; A=hi(multiplier)
        bpl     mul16_multiplicand
        nega
        neg     R2L
        sbc     #0
        sta     R2H             ; negate multiplier
mul16_multiplicand:
        lda     R1H             ; A=hi(multiplicand)
        bpl     mul16_multiply
        nega
        neg     R1L
        sbc     #0
        sta     R1H             ; negate multiplicand
mul16_multiply:
        bsr     umul16          ; R0=result
        pula                    ; A=(multiplicand^multiplier)
        tsta
        bpl     mul16_end
        lda     R0H
        nega
        neg     R0L
        sbc     #0
        sta     R0H             ; negate result
mul16_end:
        ldhx    R0
        rts

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram R1 dividend
;;; @praram R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber R2
udiv16:
        lda     R2H
        bne     udiv16_calc     ; branch if divisor >= 256
        ldhx    R1              ; H:A=dividend
        txa
        ldx     R2L             ; X=divisor
        div                     ; H:A/X = A...H
        bcs     udiv16_calc     ; branch if overflow
        clr     R0H
        sta     R0L             ; store quotient
        pshh
        pulx
        clrh
        sthx    R1              ; store remainder
        rts
udiv16_calc:
        lda     R2H
        bne     udiv16_calc     ; branch if divisor >= 256
        ora     R2L
        beq     udiv16_end      ; branch if R2==0
        ldx     #1              ; X=bits
        bra     udiv16_prep
udiv16_prep_loop:
        lsl     R2L
        rol     R2H             ; divisor <<= 1
        incx                    ; ++bits
udiv16_prep:
        lda     R2H
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
        lda     R1L
        sub     R2L
        sta     R1L
        lda     R1H
        sbc     R2H
        sta     R1H             ; dividend -= divisor
        bcs     udiv16_readd
        inc     R0L             ; quotient |= 1
        bra     udiv16_next
udiv16_readd:
        lda     R1L
        add     R2L
        sta     R1L
        lda     R1H
        adc     R2H
        sta     R1H             ; dividend += divisor
udiv16_next:
        dbnzx   udiv16_loop     ; --bits
udiv16_end:
        rts

;;; Division; dividend / divisor = quotient ... remainder
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R0 quotient
;;; @return HX quotient
;;; @return R1 remainder
;;; @clobber R2
div16:
        lda     R1H
        eor     R2H
        psha                    ; save hi(dividend^divisor)
        lda     R2H             ; A=hi(divisor)
        bpl     div16_dividend
        nega
        neg     R2L
        sbc     #0
        sta     R2H             ; negate divisor
div16_dividend:
        lda     R1H             ; A=hi(dividend)
        bpl     div16_divide
        nega
        neg     R1L
        sbc     #0
        sta     R1H             ; negate dividend
div16_divide:
        jsr     udiv16          ; R0=quotient
        pula                    ; A=(dividend^divisor)
        tsta
        bpl     div16_end
        lda     R0H
        nega
        neg     R0L
        sbc     #0
        sta     R0H             ; negate result
div16_end:
        ldhx    R0
        rts
