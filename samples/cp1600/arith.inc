;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Print signed 16-bit integer as decimal
;;; @param R0 value
;;; @clobber R0 R1 R2
print_int16:
        tstr    r0
        bpl     print_uint16
        pshr    r5
        pshr    r0
        mvii    '-', r0
        jsr     r5, putchar
        pulr    r0
        negr    r0              ; value=-value
        pulr    r5
print_uint16:
        pshr    r5
        movr    r0, r1          ; R1=value
        mvii    10, r2          ; R2=10
        jsr     r5, udiv16      ; R0=value/10, R1=value%10
        pshr    r1              ; push remainder
        tstr    r0
        beq     print_uint16_digit
        jsr     r5, print_uint16        ; next digit
print_uint16_digit:
        pulr    r0              ; pop remainder
        addi    '0', r0
        pulr    r5
        b       putchar         ; print digit and return

;;; Unsigned Divide: dividend / divisor = quotient ... remainder
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R0 quotient
;;; @return R1 remainder
;;; @clobber R2
udiv16:
        tstr    r2
        bneq    udiv16_divide
        jr      r5              ; divide by zero
udiv16_divide:
        pshr    r3
        mvii    1, r3           ; bit=1
        b       udiv16_prep
udiv16_prep_loop:
        sll     r2              ; divisor<<=1
        sll     r3              ; bit<<=1
udiv16_prep:
        cmpr    r2, r1
        bllt    udiv16_prep_end ; branch if dividend<divisor
        tstr    r2
        bpl     udiv16_prep_loop        ; branch if msb(divisor)=0
udiv16_prep_end:
        clrr    r0              ; quotient=0
udiv16_loop:
        cmpr    r2, r1
        bllt    udiv16_next     ; branch if dividend<divisor
        subr    r2, r1          ; dividend-=divisor
        addr    r3, r0          ; quotient|=bit
udiv16_next:
        slr     r2              ; divisor>>=1
        slr     r3              ; bit>>=1
        bneq    udiv16_loop     ; branch if bit!=0
        pulr    r3
        jr      r5

;;; Signed Divide: dividend / divisor = quotient
;;; @param R1 dividend
;;; @param R2 divisor
;;; @return R1 quotient
;;; @clobber R0 R2
div16:
        pshr    r5
        pshr    r3
        movr    r1, r3
        xorr    r2, r3          ; R3=sign(quotient)
        tstr    r1
        bpl     div16_divisor
        negr    r1              ; negate dividend
div16_divisor:
        tstr    r2
        bpl     div16_divide
        negr    r2              ; negate divisor
div16_divide:
        jsr     r5, udiv16
        movr    r0, r1
        tstr    r3
        bpl     div16_return
        negr    r1              ; negate quotient
div16_return:
        pulr    r3
        pulr    r7

;;; Signed Multiply: multiplicand * multiplier = product
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R1 product
;;; @clobber R0 R2
mul16:
        tstr    r2
        bpl     umul16
        negr    r2              ; negate multiplier
        negr    r1              ; negate multiplicand
;;; Unsigned Multiply: multiplicand * multiplier = product
;;; @param R1 multiplicand
;;; @param R2 multiplier
;;; @return R1 product
;;; @clobber R0 R2
umul16:
        clrr    r0              ; product=0
        b       umul16_enter_loop
umul16_loop:
        clrc
        rrc     r2              ; multiplier>>=1, C=lsb(multiplier)
        bnc     umul16_next
        addr    r1, r0          ; product+=multiplicand
umul16_next:
        sll     r1              ; multiplicand<<=1
umul16_enter_loop:
        tstr    r2
        bneq    umul16_loop     ; branch if multiplier!=0
        movr    r0, r1
        jr      r5
