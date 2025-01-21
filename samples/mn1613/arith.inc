*** -*- mode: asm; mode: flyspell-prog* -*-
*** Print signed 16-bit integer as decimal
*** @param R0 value
*** @clobber R1 R2
print_int16:
        tbit    R0, 0, nz       skip if msb(value)=1
        b       print_uint16
        push    R0
        mvi     R0, '-'
        bal     putchar
        pop     R0
        neg     R0, c           negate value
print_uint16:
        mv      R1, R0          R1=value
        mvwi    R2, 10          R2=10
        bal     udiv16          R0=value/10, R1=value%10
        push    R1              push remainder
        or      R0, R0, z       skip if value=0
        bal     print_uint16    next digit
        pop     R0              pop remainder
        awi     R0, '0'
        b       putchar         print digit and return

*** Unsigned Divide: dividend / divisor = quotient ... remainder
*** @param R1 dividend
*** @param R2 divisor
*** @return R0 quotient
*** @return R1 remainder
udiv16:
        eor     R0, R0          DR0=dividend
        push    R2              push divisor
        mv      R2, SP
        ai      R2, 1           R2 points divisor
        d       DR0, (R2)       DR0/(R2)=R0...R1
        pop     R2
        ret

*** Signed Divide: dividend / divisor = quotient
*** @param R1 dividend
*** @param R2 divisor
*** @return R1 quotient
*** @clobber R0 R2
div16:
        push    R3
        mv      R3, R1
        tbit    R1, 0, z        skip if msb(dividend)=0
        neg     R1, c           negate dividend
        eor     R3, R2          R3=sign(quotient)
        tbit    R2, 0, z        skip if msb(divisor)=0
        neg     R2, c           negate divisor
        bal     udiv16
        tbit    R3, 0, z        skip if quotient needs not be negated
        neg     R0, c           negate quotient
        mv      R1, R0
        pop     R3
        ret

*** Signed Divide: dividend / |divisor| = quotient
*** @param R1 dividend
*** @param R2 divisor
*** @return R1 quotient
*** @clobber R0 R2 STR.15
divu16:
        rbit    STR, 15         clear STR.15
        tbit    R1, 0, z        skip if msb(dividend)=0
        sbit    STR, 15         set STR.15
        tbit    R1, 0, z        skip if msb(dividend)=0
        neg     R1, c           negate dividend
        bal     udiv16
        tbit    STR, 15, z      skip if quotient needs not be negated
        neg     R0, c           negate quotient
        mv      R1, R0
        ret

*** Signed Multiply: multiplicand * multiplier = product
*** @param R1 multiplicand
*** @param R2 multiplier
*** @return R1 product
*** @clobber R0 R2
mul16:
        push    R3
        mv      R3, R1
        eor     R3, R2          R3=sign(product)
        tbit    R1, 0, z        skip if msb(multiplicand)=0
        neg     R1, c           negate multiplicand
        tbit    R2, 0, z        skip if msb(multiplier)=0
        neg     R2, c           negate multiplier
        mv      R0, R2          R0=|multiplier|
        mv      R2, SP          R2 points |multiplicand|
        push    R1              push |multiplicand|
        m       DR0, (R2)       DR0=|multiplicand|*|multiplier|
        tbit    R3, 0, z        skip if product needs not to be negated
        neg     R1, c           negate low(product)
        mv      SP, R2          discard pushed |multiplier|
        pop     R3
        ret
