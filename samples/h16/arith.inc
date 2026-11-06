;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print signed 16-bit integer as decimal
;;; @param R0 value
;;; @clobber R0
print_int16:
        tst.w   r0
        bpl     print_uint16
        mov.w   r0, @-r15
        mov.b   #'-', r0
        bsr     putchar
        mov.w   @r15+, r0
        neg.w   r0              ; negate value
;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber R0
print_uint16:
        tst.w   r0              ; test value
        beq     print_uint16_zero
print_uint16_loop:
        tst.w   r0
        beq     print_uint16_end ; branch if value == 0
        extu.l  r0
        divxu.w #10, r0         ; R0=remainder:quotient
        mov.l   r0, @-r15       ; push remainder
        bsr     print_uint16_loop
        mov.l   @r15+, r0
        swap.w  r0              ; swap word halves, R0=remainder
print_uint16_zero:
        or.b    #'0', r0
        bra     putchar
print_uint16_end:
        rts
