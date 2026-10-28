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

;;; Multiply
;;; R1 * R2 = R1
;;; @clobber R0 R2
mul16:
        mov     r1, r0          ; R0=multiplicand
        clr     r1              ; R1=product
mul16_loop:
        clc
        ror     r2              ; PSW.C=LSB of multiplier
        bcc     mul16_next
        add     r0, r1
mul16_next:
        asl     r0
        tst     r2
        bne     mul16_loop
        rts     pc

;;; Unsigned divide
;;; R1 / R2 = R1 ... R0 (R2 < 100000)
udiv16:
        mov     r3, -(sp)
        clr     r0              ; R0=remainder
        mov     #16., r3
udiv16_loop:
        asl     r1              ; PSW.C=MSB of dividend
        rol     r0
        cmp     r0, r2
        blo     udiv16_next
        sub     r2, r0
        inc     r1              ; set quotient bit
udiv16_next:
        sob     r3, udiv16_loop
        mov     (sp)+, r3
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
