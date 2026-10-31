;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print signed 16-bit integer as decimal
;;; @param R0 value
;;; @clobber R0
print_int16:
        tst.w   r0
        bpl     print_uint16
        mov.w   r0, @-sp
        mov.b   #"-", r0
        bsr     putchar
        mov.w   @sp+, r0
        neg.w   r0              ; negate value
;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber R0
print_uint16:
        stm     (r1), @-sp
        mov.w   r0, r1          ; R1=value
        beq     print_uint16_zero
        bsr     print_uint16_loop
        ldm     @sp+, (r1)
        rts
print_uint16_zero:
        mov.b   #"0", r0
        bsr     putchar
        ldm     @sp+, (r1)
        rts
;;; @param R1 value
;;; @clobber R0 R1
print_uint16_loop:
        tst.w   r1
        beq     print_uint16_end ; branch if value == 0
        clr.w   r0              ; R0:R1=value
        divxu.w #10, r0         ; R0=remainder, R1=quotient
        mov.w   r0, @-sp        ; push remainder
        bsr     print_uint16_loop
        mov.w   @sp+, r0
        add.b   #"0", r0
        bra     putchar
print_uint16_end:
        rts

;;; Multiply; the low word of a product is the same signed or unsigned
;;; R1 * R2 = R1
;;; @clobber R0
mul16:
        mov.w   r1, r0
        mulxu.w r2, r0          ; R0:R1=R0*R2
        rts

;;; Divide, truncating toward zero
;;; R1 / R2 = R1
;;; @clobber R0
div16:
        stm     (r2-r3), @-sp
        mov.w   r1, r3
        xor.w   r2, r3          ; R3.15=sign of quotient
        tst.w   r1
        bpl     div16_dividend
        neg.w   r1
div16_dividend:
        tst.w   r2
        bpl     div16_divisor
        neg.w   r2
div16_divisor:
        clr.w   r0              ; R0:R1=|dividend|
        divxu.w r2, r0          ; R0=remainder, R1=quotient
        tst.w   r3
        bpl     div16_return
        neg.w   r1
div16_return:
        ldm     @sp+, (r2-r3)
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
