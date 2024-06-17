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

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param D multiplicand
;;; @param X multiplier
;;; @return D result
umul16:
        pshs    CC,D            ; S[0]=work, S[1:2]=multiplicand
        tfr     X,D             ; A=high(multiplier)
        ldb     2,S             ; B=low(multiplicand)
        mul                     ; low(multiplicand)*high(multiplier)
        stb     ,S              ; work
        tfr     X,D             ; B=low(multiplier)
        lda     1,S             ; A=high(multiplicand)
        mul                     ; high(multiplicand)*low(multiplier)
        addb    ,S
        stb     ,S
        tfr     X,D             ; B=low(multiplier)
        lda     2,S             ; A=low(multiplicand)
        mul                     ; D=low(multiplicand)*low(multiplier)
        adda    ,S
        leas    3,S
        rts

;;; Multiply; result = multiplicand * multiplier
;;; @param D multiplicand
;;; @param X multiplier
;;; @return D result
mul16:
        pshs    D
        tfr     X,D
        eora    ,S
        pshs    A               ; save hi(multiplicand^multiplier)
        ldd     1,S
        exg     D,X
        tsta
        bpl     mul16_multiplicand
        coma
        comb
        addd    #1              ; negate multiplier
mul16_multiplicand:
        exg     D,X
        tsta
        bpl     mul16_multiply
        coma
        comb
        addd    #1              ; negate multiplicand
mul16_multiply:
        bsr     umul16          ; R0=result
        tst     ,S+             ; A=(multiplicand^multiplier)
        bpl     mul16_end
        coma
        comb
        addd    #1              ; negate result
mul16_end:
        leas    2,S
        rts

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram D dividend
;;; @praram X divisor
;;; @return D quotient
;;; @return X remainder
udiv16:
        pshs    CC,D,X          ; S[0]=bits, S[1:2]=dividend, S[3:4]=divisor
        leax    ,X              ; X=0?
        beq     udiv16_end      ; divide by zero
        ldb     #1
        stb     ,S              ; bits=1
        tfr     X,D
        bra     udiv16_prep
udiv16_prep_loop:
        lslb
        rola                    ; divisor <<= 1
        inc     ,S              ; ++bits
udiv16_prep:
        tsta
        bpl     udiv16_prep_loop ; while msb(divisor) == 0
        std     3,S              ; update divisor
        ldx     #0              ; X=quotient=0
        ldd     1,S             ; D=dividend
        bra     udiv16_enter_loop
udiv16_loop:
        lsr     3,S
        ror     4,S             ; divisor >>=1
        exg     D,X
        lslb
        rola
        exg     D,X             ; quotient <<= 1
udiv16_enter_loop:
        subd    3,S             ; dividend -= divisor
        blo     udiv16_readd
        leax    1,X             ; quotient |= 1
        bra     udiv16_next
udiv16_readd:
        addd    3,S             ; dividend += divisor
udiv16_next:
        dec     ,S              ; --bits
        bne     udiv16_loop     ; while bits != 0
udiv16_end:
        exg     D,X             ; D=quotient, X=remainder
        leas    5,S
        rts

;;; Division; dividend / divisor = quotient ... remainder
;;; @param D dividend
;;; @param X divisor
;;; @return D quotient
;;; @return X remainder
div16:
        pshs    D
        tfr     X,D
        eora    ,S
        pshs    A               ; save hi(dividend^divisor)
        ldd     1,S
        exg     D,X
        tsta
        bpl     div16_dividend
        coma
        comb
        addd    #1              ; negate divisor
div16_dividend:
        exg     D,X
        tsta
        bpl     div16_divide
        coma
        comb
        addd    #1              ; negate dividend
div16_divide:
        bsr     udiv16          ; R0=quotient
        tst     ,S+             ; A=(dividend^divisor)
        bpl     div16_end
        coma
        comb
        addd    #1              ; negate result
div16_end:
        leas    2,S
        rts
