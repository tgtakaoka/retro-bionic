;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; 16-bit integer arithmetic: the H8/300 multiplies and divides only
;;; bytes.

;;; Print signed 16-bit integer as decimal
;;; @param R0 value
;;; @clobber R0
print_int16:
        mov.w   r0, r0
        bpl     print_uint16
        push    r0
        mov.b   #"-", r0l
        jsr     @putchar
        pop     r0
        not.b   r0l
        not.b   r0h
        adds    #1, r0          ; negate value
;;; Print unsigned 16-bit integer as decimal
;;; @param R0 value
;;; @clobber R0
print_uint16:
        push    r1
        push    r2
        push    r3
        mov.w   r0, r1          ; R1=value
        bne     print_uint16_digits
        mov.b   #"0", r0l
        jsr     @putchar
        bra     print_uint16_end
print_uint16_digits:
        bsr     print_uint16_loop
print_uint16_end:
        pop     r3
        pop     r2
        pop     r1
        rts
;;; @param R1 value
;;; @clobber R0 R1 R2 R3
print_uint16_loop:
        mov.w   r1, r1
        beq     print_uint16_return ; branch if value == 0
        mov.w   #10, r2
        bsr     udiv16          ; R0=remainder, R1=quotient
        push    r0              ; push remainder
        bsr     print_uint16_loop
        pop     r0
        add.b   #"0", r0l
        jmp     @putchar
print_uint16_return:
        rts

;;; Multiply; the low word of a product is the same signed or unsigned
;;; R1 * R2 = R1
;;; @clobber R0
mul16:
        push    r3
        mov.b   r1h, r0l
        mulxu   r2l, r0         ; R0=R1H*R2L
        mov.b   r2h, r3l
        mulxu   r1l, r3         ; R3=R2H*R1L
        add.b   r3l, r0l        ; R0L=low byte of the cross products
        mulxu   r2l, r1         ; R1=R1L*R2L
        add.b   r0l, r1h
        pop     r3
        rts

;;; Unsigned divide
;;; R1 / R2 = R1 ... R0
;;; @clobber R3L
udiv16:
        sub.w   r0, r0          ; R0=0
        mov.b   #16, r3l
udiv16_loop:
        shll.b  r1l
        rotxl.b r1h             ; C=R1.15, R1<<=1
        rotxl.b r0l
        rotxl.b r0h             ; R0=R0<<1|C
        cmp.w   r2, r0
        blo     udiv16_next     ; branch if R0<R2
        sub.w   r2, r0
        inc.b   r1l             ; quotient bit
udiv16_next:
        dec.b   r3l
        bne     udiv16_loop
        rts

;;; Divide, truncating toward zero
;;; R1 / R2 = R1
;;; @clobber R0
div16:
        push    r2
        push    r3
        mov.b   r1h, r3h
        xor.b   r2h, r3h        ; R3H.7=sign of quotient
        mov.w   r1, r1
        bpl     div16_dividend
        not.b   r1l
        not.b   r1h
        adds    #1, r1          ; R1=-R1
div16_dividend:
        mov.w   r2, r2
        bpl     div16_divisor
        not.b   r2l
        not.b   r2h
        adds    #1, r2          ; R2=-R2
div16_divisor:
        bsr     udiv16
        btst    #7, r3h
        beq     div16_return
        not.b   r1l
        not.b   r1h
        adds    #1, r1          ; R1=-R1
div16_return:
        pop     r3
        pop     r2
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
