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
        mov     *R10+, R11      Pop return address
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

        *** Unsigned Divide
        *** R1 / |R2| = R1
        *** @clobber R2
divu16:
        mov     R1, R0
        sra     R0, 15          Extend sign bit
        divs    R2              R0:R1/R2=R0..R1
        mov     R0, R1
        rt

        *** Divide
        *** R1 / R2 = R1 ... R0
        *** @clobber R2
div16:
        mov     R1, R0
        sra     R0, 15          Extend sign bit
        divs    R2              R0:R1/R2=R0...R1
        mov     R1, R2
        mov     R0, R1
        mov     R2, R0
        rt
