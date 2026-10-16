;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6809

;;; Print signed 16-bit integer as decimal
;;; @param D value
;;; @clobber X
print_int16:
        pshs    D
        tsta
        bpl     print_int16_print
        lda     #'-'
        jsr     putchar
        ldd     ,S
        coma
        comb
        addd    #1              ; negate value
print_int16_print:
        bsr     print_uint16
        puls    D,PC

;;; Print unsigned 16-bit integer as decimal
;;; Recursion stacks the digits so the most significant prints first.
;;; @param D value
;;; @clobber D X
print_uint16:
        addd    #0
        beq     print_uint16_zero
print_uint16_loop:
        addd    #0
        beq     print_uint16_end ; branch if value == 0
        ldx     #10
        jsr     udiv16          ; D=value/10, X=value%10
        exg     D,X
        pshs    B               ; push remainder
        exg     D,X
        bsr     print_uint16_loop
        puls    A
print_uint16_zero:
        ora     #'0'
        jmp     putchar
print_uint16_end:
        rts

;;; Multiply; result = multiplicand * multiplier
;;; Signed and unsigned products share the low 16 bits.
;;; @param D multiplicand
;;; @param X multiplier
;;; @return D result
mul16:
umul16:
        pshs    D,X             ; S[0:1]=multiplicand, S[2:3]=multiplier
        ldb     3,S             ; B=low(multiplier)
        mul                     ; high(multiplicand)*low(multiplier)
        stb     ,S              ; work
        ldd     1,S             ; A=low(multiplicand), B=high(multiplier)
        mul                     ; low(multiplicand)*high(multiplier)
        addb    ,S
        stb     ,S
        lda     1,S             ; A=low(multiplicand)
        ldb     3,S             ; B=low(multiplier)
        mul                     ; D=low(multiplicand)*low(multiplier)
        adda    ,S
        leas    4,S
        rts

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @param D dividend
;;; @param X divisor
;;; @return D quotient
;;; @return X remainder
udiv16:
        pshs    X               ; S[0:1]=divisor
        cmpd    ,S
        blo     udiv16_small    ; dividend < divisor
        leax    ,X              ; X=0?
        beq     udiv16_small    ; divide by zero
        pshs    D               ; S[0:1]=dividend, S[2:3]=divisor
        ldd     #-1
        pshs    D               ; S[0:1]=~quotient and marker
        tfr     X,D
        tsta
        bmi     udiv16_prep_end
udiv16_prep_loop:
        lslb
        rola                    ; divisor <<= 1
        lsr     ,S
        ror     1,S             ; marker >>= 1, one 0 bit per shift
        cmpd    2,S
        bhi     udiv16_prep_end ; until divisor > dividend
        tsta
        bpl     udiv16_prep_loop ; while msb(divisor) == 0
udiv16_prep_end:
        std     4,S             ; update divisor
        ldd     2,S             ; D=dividend
        bra     udiv16_enter_loop
udiv16_loop:
        lsr     4,S
        ror     5,S             ; divisor >>= 1
udiv16_enter_loop:
        subd    4,S             ; dividend -= divisor
        bhs     udiv16_next     ; C=0
        addd    4,S             ; dividend += divisor, C=1
udiv16_next:
        rol     1,S
        rol     ,S              ; ~quotient = ~quotient<<1 | C
        bcc     udiv16_loop     ; until marker shifts out
        tfr     D,X             ; X=remainder
        ldd     ,S
        coma
        comb                    ; D=quotient
        leas    6,S
        rts
udiv16_small:
        tfr     D,X             ; X=remainder
        ldd     #0              ; D=quotient
        leas    2,S
        rts

;;; Division; dividend / divisor = quotient ... remainder
;;; @param D dividend
;;; @param X divisor
;;; @return D quotient
;;; @return X remainder of |dividend|/|divisor|
div16:
        cmpx    #0
        bmi     div16_divisor   ; divisor < 0
        tsta
        bpl     udiv16          ; dividend >= 0
        coma
        comb
        addd    #1              ; negate dividend
div16_negate:
        bsr     udiv16
        coma
        comb
        addd    #1              ; negate quotient
        rts
div16_divisor:
        exg     D,X
        coma
        comb
        addd    #1              ; negate divisor
        exg     D,X
        tsta
        bpl     div16_negate    ; dividend >= 0
        coma
        comb
        addd    #1              ; negate dividend
        bra     udiv16
