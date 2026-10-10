;;; -*- mode: asm; mode: flyspell-prog; -*-

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
        coma
        comb
        addd    #1        ; negate value
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

;;; Multiply; result = multiplicand * multiplier
;;; @param D multiplicand
;;; @param X multiplier
;;; @return D result
;;; @clobber X
mul16:
umul16:
        pshy
        tfr     x,y
        emuls                   ; Y:D=D*Y, the low half for either sign
        puly
        rts

;;; Division; dividend / divisor = quotient ... remainder
;;; @param D dividend
;;; @param X divisor
;;; @return D quotient
;;; @return X remainder
div16:
        idivs                   ; D/X, X=quotient, D=remainder
        xgdx
        rts
