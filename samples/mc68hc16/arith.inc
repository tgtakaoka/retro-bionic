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
        negd                    ; negate value
;;; Print unsigned 16-bit integer as decimal
;;; @param D value
;;; @clobber D X
print_uint16:
        tstd                    ; test value
        beq     print_uint16_zero
print_uint16_loop:
        tstd
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
