;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; The DCT11 has no MUL nor DIV.

;;; Print signed 16-bit integer as decimal
;;; @param R0 value
print_int16:
        mov     r0, -(sp)
        mov     r1, -(sp)
        mov     r2, -(sp)
        mov     r0, r1
        beq     print_int16_zero
        bpl     print_uint16
        movb    #'-, r0
        jsr     pc, putchar
        neg     r1
print_uint16:
        mov     #10., r2
        jsr     pc, print_uint16_loop
print_int16_return:
        mov     (sp)+, r2
        mov     (sp)+, r1
        mov     (sp)+, r0
        rts     pc
print_int16_zero:
        movb    #'0, r0
        jsr     pc, putchar
        br      print_int16_return

;;; @param R1 non-zero value
;;; @param R2 10
;;; @clobber R0 R1
print_uint16_loop:
        jsr     pc, udiv16      ; R1=quotient R0=remainder
        mov     r0, -(sp)       ; push remainder
        tst     r1
        beq     print_uint16_digit
        jsr     pc, print_uint16_loop
print_uint16_digit:
        mov     (sp)+, r0       ; pop remainder
        add     #'0, r0
        jmp     putchar

;;; Signed multiply
;;; R1 * R2 = R1
;;; @clobber R0 R2
;;; The low 16 bits of a product are the same signed or unsigned, and
;;; negating both operands keeps it: a negative multiplier is negated only
;;; so that the loop below, which ends when it is 0, takes fewer turns.
mul16:
        tst     r2
        bpl     umul16
        neg     r2              ; negate multiplier
        neg     r1              ; negate multiplicand
;;; Unsigned multiply
;;; R1 * R2 = R1
;;; @clobber R0 R2
umul16:
        mov     r1, r0          ; R0=multiplicand
        clr     r1              ; R1=product
        br      umul16_enter
umul16_loop:
        clc
        ror     r2              ; multiplier>>=1, C=lsb(multiplier)
        bcc     umul16_next
        add     r0, r1          ; product+=multiplicand
umul16_next:
        asl     r0              ; multiplicand<<=1
umul16_enter:
        tst     r2
        bne     umul16_loop     ; branch if multiplier!=0
        rts     pc

;;; Unsigned divide
;;; R1 / R2 = R1 ... R0
;;; The divisor is shifted up past the dividend first, so the loop takes
;;; only as many turns as the quotient has bits.
udiv16:
        mov     r1, r0          ; R0=dividend
        clr     r1              ; R1=quotient
        tst     r2
        beq     udiv16_zero     ; divide by zero
        mov     r3, -(sp)
        mov     r2, -(sp)
        mov     #1, r3          ; R3=bit
        br      udiv16_prep
udiv16_prep_loop:
        asl     r2              ; divisor<<=1
        asl     r3              ; bit<<=1
udiv16_prep:
        cmp     r2, r0
        bhi     udiv16_loop     ; branch if dividend<divisor
        tst     r2
        bpl     udiv16_prep_loop        ; branch if msb(divisor)=0
udiv16_loop:
        cmp     r2, r0
        bhi     udiv16_next     ; branch if dividend<divisor
        sub     r2, r0          ; dividend-=divisor
        bis     r3, r1          ; quotient|=bit
udiv16_next:
        clc
        ror     r2              ; divisor>>=1
        clc
        ror     r3              ; bit>>=1
        bne     udiv16_loop     ; branch if bit!=0
        mov     (sp)+, r2
        mov     (sp)+, r3
udiv16_zero:
        rts     pc

;;; Divide by positive divisor
;;; R1 / R2 = R1 (R2 > 0)
;;; @clobber R0
divu16:
        tst     r1
        bpl     udiv16
        neg     r1
        jsr     pc, udiv16
        neg     r1
        rts     pc

;;; Divide
;;; R1 / R2 = R1 ... R0 (R0=|remainder|)
;;; @clobber R2
div16:
        mov     r1, r0
        xor     r2, r0
        mov     r0, -(sp)       ; push R1^R2
        tst     r1
        bpl     div16_divisor
        neg     r1
div16_divisor:
        tst     r2
        bpl     div16_divide
        neg     r2
div16_divide:
        jsr     pc, udiv16
        tst     (sp)+           ; pop R1^R2
        bpl     div16_return
        neg     r1
div16_return:
        rts     pc
