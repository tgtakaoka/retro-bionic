;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print signed 16-bit integer as decimal
;;; @param D0.W value
;;; @clobber D0
print_int16:
        tst.w   d0
        bpl.s   print_uint16
        move.w  d0,-(a7)
        move.b  #'-',d0
        bsr.w   putchar
        move.w  (a7)+,d0
        neg.w   d0              ; negate value
;;; Print unsigned 16-bit integer as decimal
;;; @param D0.W value
;;; @clobber D0
print_uint16:
        and.l   #$FFFF,d0
        bne.s   print_uint16_loop
        move.b  #'0',d0
        bra.w   putchar
print_uint16_loop:
        tst.l   d0
        beq.s   print_uint16_end ; branch if value == 0
        divu    #10,d0          ; D0.W=quotient, D0 high=remainder
        swap    d0
        move.w  d0,-(a7)        ; push remainder
        clr.w   d0
        swap    d0              ; D0=quotient
        bsr.s   print_uint16_loop
        move.w  (a7)+,d0
        or.b    #'0',d0
        bra.w   putchar
print_uint16_end:
        rts
