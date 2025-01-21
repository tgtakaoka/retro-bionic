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
        eor     R1, R1          R1=0
        s       R1, R0          R1=-value
        mv      R0, R1
print_uint16:
        mv      R1, R0          R1=value
        eor     R2, R2
        mvi     R2, 10          R2=10
        bal     udiv16          R0=value/10, R1=value%10
        push    R1              push remainder
        or      R0, R0, z       skip if value=0
        bal     print_uint16    next digit
        pop     R0              pop remainder
        mvi     R2, '0'
        a       R0, R2
        b       putchar         print digit and return

*** Unsigned Divide: dividend / divisor = quotient ... remainder
*** @param R1 dividend
*** @param R2 divisor
*** @return R0 quotient
*** @return R1 remainder
*** @clobber R2
udiv16:
        or      R2, R2, nz      Skip if R2 != 0
        ret                     Divide by zero
        push    R3
        eor     R3, R3
        mvi     R3, 1           bits=1
        b       udiv16_prep
udiv16_prep_loop:
        sl      R2, re          divisor<<=1
        ai      R3, 1           bits++
udiv16_prep:
        tbit    R2, 0, nz       Skip if msb(R2)==1
        b       udiv16_prep_loop
        eor     R0, R0          quotient=0 (R0)
        b       udiv16_enter_loop
udiv16_loop:
        sr      R2, re          divisor>>=1
        sl      R0, re          quotient<<=1
udiv16_enter_loop:
        s       R1, R2, lpz     dividend-=divisor, skip if dividend>=divisor
        b       udiv16_readd
        ai      R0, 1           quotient+=1
        b       udiv16_enter_loop
udiv16_readd:
        a       R1, R2          dividend+=divisor
        si      R3, 1, z        bits--, skip if bits==0
        b       udiv16_loop
        pop     R3
        ret

*** Signed Divide: dividend / divisor = quotient
*** @param R1 dividend
*** @param R2 divisor
*** @return R1 quotient
*** @clobber R0 R2
div16:
        push    R3
        mv      R3, R1
        tbit    R1, 0, nz       skip if msb(dividend)=1
        b       div16_divisor
        eor     R0, R0
        s       R0, R1
        mv      R1, R0          negate dividend
div16_divisor:
        eor     R3, R2          R3=sign(quotient)
        tbit    R2, 0, nz       skip if msb(divisor)=1
        b       div16_divide
        eor     R0, R0
        s       R0, R2
        mv      R2, R0          negate divisor
div16_divide:
        bal     udiv16
        tbit    R3, 0, nz       skip if quotient must be negated
        b       div16_return
        eor     R1, R1
        s       R1, R0, skp     negate quotient
div16_return:
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
        tbit    R1, 0, nz       skip if msb(dividend)=1
        b       divu16_divide
        sbit    STR, 15         set STR.15
        eor     R0, R0
        s       R0, R1
        mv      R1, R0          negate dividend
divu16_divide:
        bal     udiv16
        tbit    STR, 15, nz     skip if quotient must be negated
        b       divu16_return
        eor     R1, R1
        s       R1, R0, skp     negate quotient
divu16_return:
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
        tbit    R1, 0, nz       skip if msb(multiplicand)=1
        b       mul16_multiplier
        eor     R0, R0
        s       R0, R1
        mv      R1, R0          negate multiplicand
mul16_multiplier:
        tbit    R2, 0, nz       skip if msb(multiplier)=1
        b       mul16_divide
        eor     R0, R0
        s       R0, R2
        mv      R2, R0          negate multiplier
mul16_divide:
        bal     umul16
        tbit    R3, 0, nz       skip if product must be negated
        b       mul16_return
        eor     R1, R1
        s       R1, R0, skp     negate product
mul16_return:
        mv      R1, R0
        pop     R3
        ret

*** Unsigned Multiply: multiplicand * multiplier = product
*** @param R1 multiplicand
*** @param R2 multiplier
*** @return R0 product
*** @clobber R1 R2
umul16:
        eor     R0, R0          product=0
        b       umul16_enter_loop
umul16_loop:
        sr      R2, re, ez      multiplier>>=1, skip if lsb(multiplier)=0
        a       R0, R1          product+=multiplicand
        sl      R1, re          multiplicand<<=1
umul16_enter_loop:
        or      R2, R2, z       skip if multiplier=0
        b       umul16_loop
        ret
