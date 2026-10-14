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
        mvi     R3, 1           bit=1
        b       udiv16_prep
udiv16_prep_loop:
        sl      R2, re          divisor<<=1
        sl      R3, re          bit<<=1
udiv16_prep:
        c       R1, R2, lpz     Skip if dividend>=divisor
        b       udiv16_prep_end
        tbit    R2, 0, nz       Skip if msb(R2)==1
        b       udiv16_prep_loop
udiv16_prep_end:
        eor     R0, R0          quotient=0 (R0)
udiv16_loop:
        s       R1, R2, lpz     dividend-=divisor, skip if dividend>=divisor
        b       udiv16_readd
        or      R0, R3, skp     quotient|=bit
udiv16_readd:
        a       R1, R2          dividend+=divisor
        sr      R2, re          divisor>>=1
        sr      R3, re, z       bit>>=1, skip if bit==0
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
*** @clobber R0
divu16:
        eor     R0, R0          quotient=0
        tbit    R1, 0, z        skip if dividend>=0
        b       divu16_neg
        c       R1, R2, lm      skip if dividend<divisor
        b       divu16_shift
        mv      R1, R0          quotient=0
        ret
*** Shift divisor up until it exceeds dividend, then enter the step
*** that shifts it back down; dividend<X'8000' bounds this to 14.
divu16_shift:
        sl      R2, re          divisor<<=1
        c       R1, R2, lpz     skip if dividend>=divisor
        b       divu16_1
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_2
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_3
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_4
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_5
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_6
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_7
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_8
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_9
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_10
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_11
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_12
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_13
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_14
*** One restoring step per quotient bit, each setting its own bit.
        s       R1, R2, lm      dividend-=divisor, skip if borrow
        sbit    R0, 1, skp      set quotient bit, skip the restore
        a       R1, R2          restore dividend
divu16_14:
        sr      R2, re          divisor>>=1
        s       R1, R2, lm
        sbit    R0, 2, skp
        a       R1, R2
divu16_13:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 3, skp
        a       R1, R2
divu16_12:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 4, skp
        a       R1, R2
divu16_11:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 5, skp
        a       R1, R2
divu16_10:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 6, skp
        a       R1, R2
divu16_9:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 7, skp
        a       R1, R2
divu16_8:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 8, skp
        a       R1, R2
divu16_7:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 9, skp
        a       R1, R2
divu16_6:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 10, skp
        a       R1, R2
divu16_5:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 11, skp
        a       R1, R2
divu16_4:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 12, skp
        a       R1, R2
divu16_3:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 13, skp
        a       R1, R2
divu16_2:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 14, skp
        a       R1, R2
divu16_1:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 15, skp
        a       R1, R2
        mv      R1, R0
        ret
*** Negative dividend: divide -dividend, then negate the quotient.
divu16_neg:
        s       R0, R1          R0=-dividend
        c       R0, R2, lpz     skip if -dividend>=divisor
        b       divu16_n_zero
        mv      R1, R0, pz      skip if -dividend<X'8000'
        b       divu16_min
        eor     R0, R0          quotient=0
divu16_n_shift:
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_1
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_2
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_3
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_4
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_5
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_6
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_7
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_8
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_9
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_10
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_11
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_12
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_13
        sl      R2, re
        c       R1, R2, lpz
        b       divu16_n_14
        s       R1, R2, lm
        sbit    R0, 1, skp
        a       R1, R2
divu16_n_14:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 2, skp
        a       R1, R2
divu16_n_13:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 3, skp
        a       R1, R2
divu16_n_12:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 4, skp
        a       R1, R2
divu16_n_11:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 5, skp
        a       R1, R2
divu16_n_10:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 6, skp
        a       R1, R2
divu16_n_9:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 7, skp
        a       R1, R2
divu16_n_8:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 8, skp
        a       R1, R2
divu16_n_7:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 9, skp
        a       R1, R2
divu16_n_6:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 10, skp
        a       R1, R2
divu16_n_5:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 11, skp
        a       R1, R2
divu16_n_4:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 12, skp
        a       R1, R2
divu16_n_3:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 13, skp
        a       R1, R2
divu16_n_2:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 14, skp
        a       R1, R2
divu16_n_1:
        sr      R2, re
        s       R1, R2, lm
        sbit    R0, 15, skp
        a       R1, R2
        eor     R1, R1
        s       R1, R0          R1=-quotient
        ret
divu16_n_zero:
        eor     R1, R1          quotient=0
        ret
divu16_min:
        push    R2
        bal     (I_udiv16)      dividend=-32768
        pop     R2
        eor     R1, R1
        s       R1, R0          R1=-quotient
        ret
I_udiv16:       dc      udiv16

*** Signed Multiply: multiplicand * multiplier = product
*** @param R1 multiplicand
*** @param R2 multiplier
*** @return R1 product
*** @clobber R0 R2
mul16:
        mv      R0, R1          R0=multiplicand
        eor     R1, R1          product=0
        tbit    R2, 0, z        skip if multiplier>=0
        b       mul16_neg
*** One step per multiplier bit, until no 1 bits are left.
        tbit    R2, 15, z       skip if lsb(multiplier)=0
        a       R1, R0          product+=multiplicand
        sr      R2, re, nz      multiplier>>=1, skip if multiplier!=0
        ret
        sl      R0, re          multiplicand<<=1
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        a       R1, R0
        ret
*** Multiply by -multiplier, subtracting the partial products.
mul16_neg:
        s       R1, R2
        mv      R2, R1          negate multiplier
        eor     R1, R1          product=0
        tbit    R2, 15, z       skip if lsb(multiplier)=0
        s       R1, R0          product-=multiplicand
        sr      R2, re, nz      multiplier>>=1, skip if multiplier!=0
        ret
        sl      R0, re          multiplicand<<=1
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        sr      R2, re, nz
        ret
        sl      R0, re
        tbit    R2, 15, z
        s       R1, R0
        ret
