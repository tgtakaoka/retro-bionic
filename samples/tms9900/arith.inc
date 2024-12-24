*** -*- mode: asm; mode: flyspell-prog* -*-
        *** Print signed 16-bit integer as decimal
        *** @param R0 value
print_int16:
        dect    R10
        mov     R11, *R10       Push return address
        dect    R10
        mov     R0, *R10        Push R0
        jeq     print_int16_zero
        jgt     print_uint16
        li      R0, hibyte('-')
        bl      @putchar
        mov     *R10, R0        Restore R0
        neg     R0
print_uint16:
        dect    R10
        mov     R1, *R10        Push R1
        dect    R10
        mov     R2, *R10        Push R2
        li      R2, 10
        bl      @print_uint16_loop
        mov     *R10+, R2       Pop R2
        mov     *R10+, R1       Pop R1
print_int16_return:
        mov     *R10+, R0       Pop R0
        mov     *R10+, R11      Pop return adddress
        rt
print_int16_zero:
        li      R0, hibyte('0')
        bl      @putchar
        jmp     print_int16_return

print_uint16_loop:
        dect    R10
        mov     R11, *R10       Push return address
        mov     R0, R1
        jeq     print_uint16_return
        clr     R0
        div     R2, R0          R0:R1/R2 ; R0=quotient R1=reminder
        dect    R10
        mov     R1, *R10        Push reminder
        bl      @print_uint16_loop
        mov     *R10+, R0       Pop reminder
        swpb    R0
        ai      R0, hibyte('0')
        bl      @putchar
print_uint16_return:
        mov     *R10+, R11      Pop return address
        rt

        *** Multiply
        *** R1 * R2 = R1
        *** @clobber R0 R2
mul16:
        mov     R2, R0
        xor     R1, R2          R2=multiplicand^multiplyer
        abs     R0
        abs     R1
        mpy     R1, R0          R0:R1=R0*R1
        li      R0, >8000
        szc     R2, R0
        jne     mul16_return
        neg     R1
mul16_return:
        rt

        *** Unsigned Divide
        *** R1 / |R2| = R1
        *** @clobber R2, R0
udiv16:
        clr     R0
        ci      R1, 0
        jlt     udiv16_negative R1 < 0
        div     R2, R0          R0:R1/R2=R0..R1
        mov     R0, R1
        rt
udiv16_negative:
        abs     R1
        div     R2, R0          R0:R1/R2=R0..R1
        neg     R0
        mov     R0, R1
        rt

        *** Divide
        *** R1 / R2 = R1 ... R0
        *** @clobber R2
div16:
        mov     R1, R0
        xor     R2, R0
        dect    R10
        mov     R0, *R10        Push R1^R2
        clr     R0
        abs     R1
        abs     R2
        div     R2, R0          R0:R1/R2=R0..R1
        li      R2, >8000
        szc     *R10+, R2       Pop R1^R2
        jne     div16_return
        neg     R0
div16_return:
        mov     R0, R2          Exchange R0, R1
        mov     R1, R0
        mov     R2, R1
        rt
