;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     2650
;;; PSL.WC and PSL.COM must be 1

;;; Print unsigned 16-bit integer as decimal
;;; @param R0:R1: value
;;; @clobber R0 R1
print_uint16:
        strr,r2 print_uint16_r2
        lodi,r2 0
        strr,r2 print_uint16_buf
print_uint16_inner:
        stra,r0 udiv16_dividend
        stra,r1 udiv16_dividend+1
print_uint16_loop:
        eorz    r0
        stra,r0 udiv16_divisor
        lodi,r0 10
        stra,r0 udiv16_divisor+1
        bsta,un udiv16            ; R0:R1=value / 10
        stra,r0 udiv16_dividend
        loda,r0 udiv16_reminder+1 ; R0=value % 10
        stra,r1 udiv16_dividend+1
        iori,r0 A'0'
        stra,r0 print_uint16_buf,r2,+ ; buf[++R2]=R0+'0'
        lodz    r1
        iora,r0 udiv16_dividend
        bcfr,eq print_uint16_loop
        cpsl    PSL_C           ; PSL.C=0
        addi,r2 1
print_uint16_out:
        loda,r0 print_uint16_buf,r2,-
        bctr,eq print_uint16_end
        bsta,un putchar
        bctr,un print_uint16_out
print_uint16_end:
        lodr,r2 print_uint16_r2
        retc,un
print_uint16_r2:
        res     1
print_uint16_buf:
        res     8

;;; Print signed 16-bit integer as decimal
;;; @param R0:R1 value
;;; @clobber R0 R1
print_int16:
        iorz    r0
        bcfa,lt print_uint16    ; branch if R0:R1>=0
        bsta,un negsi1          ; R0:R1=-value
        strr,r0 print_uint16_buf
        lodi,r0 A'-'
        bsta,un putchar
        lodr,r0 print_uint16_buf
        bcta,un print_uint16

;;; Negation; result = -value
;;; @param @R2: result
;;; @param @R3: value
;;; @clobber R0 R1 PSL
negsi2:
        loda,r0 arith_work+1,r3
        strz    r1
        loda,r0 arith_work,r3
        bstr,un negsi1
        stra,r0 arith_work,r2
        lodz    r1
        stra,r0 arith_work+1,r2
        retc,un

;;; Negation; result = -result
;;; @param R0:R1 result
;;; @clobber R0 R1 PSL
negsi1:
        eori,r1 H'FF'
        cpsl    PSL_C           ; PSL.C=0
        addi,r1 1
        eori,r0 H'FF'
        addi,r0 0               ; with carry
        retc,un

;;; Signed addition: summand += addend
;;; @param @R2: summand
;;; @param @R3: addend
;;; @clobber R0 PSL
addsi2:
        loda,r0 arith_work+1,r2
        cpsl    PSL_C           ; PSL.C=0
        adda,r0 arith_work+1,r3
        stra,r0 arith_work+1,r2
        loda,r0 arith_work,r2
        adda,r0 arith_work,r3  ; with carry
        stra,r0 arith_work,r2
        retc,un

;;; Signed subtraction: minuend -= subtrahend
;;; @param @R2: minuend
;;; @param @R3: subtrahend
;;; @clobber R0 PSL
subsi2:
        loda,r0 arith_work+1,r2
        ppsl    PSL_C           ; PSL.C=1
        suba,r0 arith_work+1,r3
        stra,r0 arith_work+1,r2
        loda,r0 arith_work,r2
        suba,r0 arith_work,r3  ; with borrow
        stra,r0 arith_work,r2
        retc,un

;;; Signed comparison: minuend - subtrahend
;;; @param @R2: minuend
;;; @param @R3: subtrahend
;;; @clobber R0 R1 PSL
cmpsi2:
        loda,r0 arith_work+1,r2
        ppsl    PSL_C           ; PSL.C=1
        suba,r0 arith_work+1,r3
        strz    r1              ; R1=low(@r2-@r3)
        loda,r0 arith_work,r2
        suba,r0 arith_work,r3
        strr,r0 cmpsi2_hi       ; R0=high(@r2-@r3)
        iorz    r1              ; R0=low(@r2-@r3)|high(@r2-@r3)
        retc,eq                 ; return if PSL.CC=Z
        lodr,r0 cmpsi2_hi       ; R0.7=N
        tpsl    PSL_OVF
        bcfr,eq cmpsi2_v        ; branch if PSL.OVF=0
        eori,r0 H'80'           ; R0.7=N^OVF
cmpsi2_v:
        iori,r0 H'01'           ; PSL.CC!=Z
        retc,un
cmpsi2_hi:
        res     1

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param mul16_multiplicand
;;; @param mul16_multiplier
;;; @return R0:R1 result
;;; @clobber R0 R1 PSL
umul16:
        stra,r2 umul16_r2
        stra,r3 umul16_r3
        eorz    r0
        strz    r1              ; r0:r1=result
;;; hi(multiplier) only adds lo(multiplicand) into hi(result)
        lodr,r2 umul16_multiplier
        bctr,eq umul16_lo       ; branch if hi(multiplier) == 0
        lodr,r3 umul16_multiplicand+1
        bctr,un umul16_hi_enter
umul16_hi_loop:
        cpsl    PSL_C           ; PSL.C=0
        rrl     r3              ; lo(multiplicand) <<= 1
umul16_hi_enter:
        cpsl    PSL_C           ; PSL.C=0
        rrr     r2              ; hi(multiplier) >>= 1
        tpsl    PSL_C
        bcfr,eq umul16_hi_next  ; branch if PSL.C=0
        cpsl    PSL_C           ; PSL.C=0
        addz    r3              ; hi(result) += lo(multiplicand)
umul16_hi_next:
        brnr,r2 umul16_hi_loop  ; while hi(multiplier) != 0
umul16_lo:
        lodr,r2 umul16_multiplier+1
        bctr,eq umul16_end      ; branch if lo(multiplier) == 0
        bctr,un umul16_lo_enter
umul16_lo_loop:
        lodr,r3 umul16_multiplicand+1
        cpsl    PSL_C           ; PSL.C=0
        rrl     r3
        strr,r3 umul16_multiplicand+1
        lodr,r3 umul16_multiplicand
        rrl     r3
        strr,r3 umul16_multiplicand ; multiplicand <<= 1
umul16_lo_enter:
        cpsl    PSL_C           ; PSL.C=0
        rrr     r2              ; lo(multiplier) >>= 1
        tpsl    PSL_C
        bcfr,eq umul16_lo_next  ; branch if PSL.C=0
        cpsl    PSL_C           ; PSL.C=0
        addr,r1 umul16_multiplicand+1
        addr,r0 umul16_multiplicand ; result += multiplicand
umul16_lo_next:
        brnr,r2 umul16_lo_loop  ; while lo(multiplier) != 0
umul16_end:
        lodr,r2 umul16_r2
        lodr,r3 umul16_r3
        retc,un

umul16_multiplicand:
        res     2
umul16_multiplier:
        res     2
umul16_r2:
        res     1
umul16_r3:
        res     1

mul16_hi:
        res     1
mul16_sign:
        res     1

;;; Signed multiplication: result = multiplicand * multiplier
;;; @param R0:R1 multiplicand
;;; @param R2:R3 multiplier
;;; @return R0:R1 result
;;; @clobber R2 R3 PSL
mul16:
        brnr,r2 mul16_signed    ; branch if hi(multiplier) != 0
        brnr,r3 mul16_unsigned  ; branch if multiplier != 0
        eorz    r0
        strz    r1              ; result=0
        retc,un
mul16_signed:
        strr,r0 mul16_hi
        eorz    r2
        strr,r0 mul16_sign      ; sign of result
        tmi,r2  H'80'
        bcfr,eq mul16_plus      ; branch if multiplier >= 0
        eori,r3 H'FF'
        cpsl    PSL_C           ; PSL.C=0
        addi,r3 1
        eori,r2 H'FF'
        addi,r2 0               ; multiplier=-multiplier
mul16_plus:
        lodr,r0 mul16_hi
        bctr,un mul16_test
mul16_unsigned:
        strr,r0 mul16_sign      ; sign of result is the multiplicand's
        iorz    r0
mul16_test:
        bcfr,lt mul16_abs       ; branch if multiplicand >= 0
        eori,r1 H'FF'
        cpsl    PSL_C           ; PSL.C=0
        addi,r1 1
        eori,r0 H'FF'
        addi,r0 0               ; multiplicand=-multiplicand
mul16_abs:
        bcfa,eq mul16_wide      ; branch if multiplicand >= 256
        brna,r2 mul16_wide      ; branch if multiplier >= 256
;;; 8x8 multiply: R0 accumulates hi(result) while R1 shifts the
;;; multiplicand out and lo(result) in, both through PSL.C. The PSL.C
;;; rotated into R1 first leaves it last, untested.
mul16_8x8:
        bdrr,r3 mul16_byte      ; R3=multiplier-1, the set PSL.C adds 1
mul16_byte:
        comi,r1 32
        bcta,lt mul16_short     ; branch if multiplicand < 32
        rrr     r1              ; PSL.C=multiplicand bit 0
        tpsl    PSL_C
        bcfr,eq mul16_b0        ; branch if PSL.C=0
        addz    r3              ; hi(result) += multiplier
mul16_b0:
        rrr     r0
        rrr     r1              ; result >>= 1, PSL.C=next multiplicand bit
        tpsl    PSL_C
        bcfr,eq mul16_b1
        addz    r3
mul16_b1:
        rrr     r0
        rrr     r1
        tpsl    PSL_C
        bcfr,eq mul16_b2
        addz    r3
mul16_b2:
        rrr     r0
        rrr     r1
        tpsl    PSL_C
        bcfr,eq mul16_b3
        addz    r3
mul16_b3:
        rrr     r0
        rrr     r1
        tpsl    PSL_C
        bcfr,eq mul16_b4
        addz    r3
mul16_b4:
        rrr     r0
        rrr     r1
        tpsl    PSL_C
        bcfr,eq mul16_b5
        addz    r3
mul16_b5:
        rrr     r0
        rrr     r1
        tpsl    PSL_C
        bcfr,eq mul16_b6
        addz    r3
mul16_b6:
        rrr     r0
        rrr     r1
        tpsl    PSL_C
        bcfr,eq mul16_b7
        addz    r3
mul16_b7:
        rrr     r0
        rrr     r1              ; R0:R1=multiplicand*multiplier
        loda,r2 mul16_sign
        bcta,lt negsi1          ; result=-result
        retc,un
;;; A multiplicand below 32 has its top 3 bits 0, which only shift
mul16_short:
        rrr     r1
        tpsl    PSL_C
        bcfr,eq mul16_s0
        addz    r3
mul16_s0:
        rrr     r0
        rrr     r1
        tpsl    PSL_C
        bcfr,eq mul16_s1
        addz    r3
mul16_s1:
        rrr     r0
        rrr     r1
        tpsl    PSL_C
        bcfr,eq mul16_s2
        addz    r3
mul16_s2:
        rrr     r0
        rrr     r1
        tpsl    PSL_C
        bcfr,eq mul16_s3
        addz    r3
mul16_s3:
        rrr     r0
        rrr     r1
        tpsl    PSL_C
        bcfr,eq mul16_s4
        addz    r3
mul16_s4:
        rrr     r0
        rrr     r1
        rrr     r0
        rrr     r1
        rrr     r0
        rrr     r1
        rrr     r0
        rrr     r1              ; R0:R1=multiplicand*multiplier
        loda,r2 mul16_sign
        bcta,lt negsi1          ; result=-result
        retc,un
;;; Square: result = value * value
;;; @param R0:R1 value
;;; @return R0:R1 result
;;; @clobber R2 R3 PSL
sqr16:
        iorz    r0
        bcfr,lt sqr16_abs       ; branch if value >= 0
        eori,r1 H'FF'
        cpsl    PSL_C           ; PSL.C=0
        addi,r1 1
        eori,r0 H'FF'
        addi,r0 0               ; value=-value
sqr16_abs:
        bcfr,eq sqr16_wide      ; branch if value >= 256
        brnr,r1 sqr16_byte
        retc,un                 ; result=0
sqr16_byte:
        stra,r0 mul16_sign      ; result >= 0
        lodz    r1
        strz    r3              ; R3=value
        eorz    r0
        bcta,un mul16_8x8
sqr16_wide:
        strz    r2
        lodz    r1
        strz    r3
        lodz    r2              ; R2:R3=value
        bcta,un mul16_unsigned
mul16_wide:
        iorz    r0
        bcfr,eq mul16_long      ; branch if multiplicand >= 256
        stra,r0 umul16_multiplier
        lodz    r1
        stra,r0 umul16_multiplier+1
        lodz    r2
        stra,r0 umul16_multiplicand
        lodz    r3
        stra,r0 umul16_multiplicand+1
        bctr,un mul16_umul      ; the multiplicand is the shorter multiplier
mul16_long:
        stra,r0 umul16_multiplicand
        lodz    r1
        stra,r0 umul16_multiplicand+1
        lodz    r2
        stra,r0 umul16_multiplier
        lodz    r3
        stra,r0 umul16_multiplier+1
mul16_umul:
        bsta,un umul16          ; R0:R1=multiplicand*multiplier
        loda,r2 mul16_sign
        bcta,lt negsi1          ; result=-result
        retc,un

;;; Signed multiplication: multiplicand *= multiplier
;;; @param @R2: multiplicand
;;; @param @R3: multiplier
;;; @clobber R0 R1 PSL
mulsi2:
        strr,r2 mulsi2_r2
        strr,r3 mulsi2_r3
        loda,r0 arith_work,r3
        strr,r0 mulsi2_hi
        loda,r0 arith_work+1,r3
        strz    r3
        loda,r0 arith_work+1,r2
        strz    r1
        loda,r0 arith_work,r2
        lodr,r2 mulsi2_hi       ; R0:R1=multiplicand, R2:R3=multiplier
        bsta,un mul16
        lodr,r2 mulsi2_r2
        lodr,r3 mulsi2_r3
        stra,r0 arith_work,r2
        lodz    r1
        stra,r0 arith_work+1,r2
        retc,un
mulsi2_hi:
        res     1
mulsi2_r2:
        res     1
mulsi2_r3:
        res     1

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @param udiv16_dividend
;;; @param udiv16_divisor
;;; @return udiv16_reminder remainder
;;; @return R0:R1 quotient
;;; @clobber R0 R1 PSL
udiv16:
        loda,r0 udiv16_divisor
        iora,r0 udiv16_divisor+1
        retc,eq                 ; divide by zero
        loda,r1 udiv16_dividend+1
        ppsl    PSL_C           ; PSL.C=1
        suba,r1 udiv16_divisor+1
        loda,r0 udiv16_dividend
        suba,r0 udiv16_divisor  ; dividend - divisor
        tpsl    PSL_C
        bctr,eq udiv16_divide   ; branch if dividend >= divisor (PSL.C=1)
        eorz    r0
        strz    r1              ; R0:R1=0, remainder=dividend
        retc,un
udiv16_divide:
        stra,r2 udiv16_r2
        stra,r3 udiv16_r3
        eorz    r0
        stra,r0 udiv16_quotient ; hi(quotient)=0
        loda,r0 udiv16_dividend
        loda,r1 udiv16_dividend+1
        cpsl    PSL_C           ; PSL.C=0
        rrr     r0
        rrr     r1
        stra,r0 udiv16_half
        stra,r1 udiv16_half+1   ; half = dividend >> 1
        rrl     r1
        rrl     r0              ; R0:R1=remainder=dividend
        lodi,r3 1               ; R3=bits
udiv16_prep:
        lodr,r2 udiv16_half+1
        ppsl    PSL_C           ; PSL.C=1
        subr,r2 udiv16_divisor+1
        lodr,r2 udiv16_half
        subr,r2 udiv16_divisor  ; half - divisor
        tpsl    PSL_C
        bcfr,eq udiv16_prep_end ; until divisor > half (PSL.C=0)
        lodr,r2 udiv16_divisor+1
        cpsl    PSL_C           ; PSL.C=0
        rrl     r2
        strr,r2 udiv16_divisor+1
        lodr,r2 udiv16_divisor
        rrl     r2
        strr,r2 udiv16_divisor  ; divisor <<= 1
        birr,r3 udiv16_prep     ; bits += 1
udiv16_prep_end:
        strr,r3 udiv16_bits
        ppsl    PSL_C           ; PSL.C=1
        subr,r1 udiv16_divisor+1
        subr,r0 udiv16_divisor  ; remainder -= divisor
        lodi,r2 0
        rrl     r2              ; R2=quotient
        bcfr,eq udiv16_halve    ; branch if remainder >= divisor
        cpsl    PSL_C           ; PSL.C=0
        addr,r1 udiv16_divisor+1
        addr,r0 udiv16_divisor  ; remainder += divisor
udiv16_halve:
        bdrr,r3 udiv16_halve_divisor ; branch if bits > 1
        bcta,un udiv16_store

udiv16_dividend:
udiv16_reminder:
        res     2
udiv16_divisor:
        res     2
udiv16_half:
        res     2
udiv16_quotient:
        res     1               ; hi(quotient)
udiv16_bits:
        res     1
udiv16_r2:
        res     1
udiv16_r3:
        res     1

;;; Halve the divisor once, then shift the remainder instead
udiv16_halve_divisor:
        lodr,r3 udiv16_divisor
        cpsl    PSL_C           ; PSL.C=0
        rrr     r3
        strr,r3 udiv16_divisor
        lodr,r3 udiv16_divisor+1
        rrr     r3
        strr,r3 udiv16_divisor+1 ; divisor >>= 1
        lodr,r3 udiv16_bits
        comi,r3 9
        bctr,lt udiv16_lo_bits  ; branch if bits < 9
        ppsl    PSL_C           ; PSL.C=1
        subi,r3 9               ; R3=bits-9 into hi(quotient)
        bcfa,eq udiv16_hi_enter
        strr,r2 udiv16_quotient
        lodi,r3 8
        bctr,un udiv16_lo_enter
udiv16_lo_bits:
        bdrr,r3 udiv16_lo_enter ; R3=bits-1 into lo(quotient)
udiv16_lo_loop:
        cpsl    PSL_C           ; PSL.C=0
        rrl     r1
        rrl     r0              ; remainder <<= 1
udiv16_lo_enter:
        ppsl    PSL_C           ; PSL.C=1
        subr,r1 udiv16_divisor+1
        subr,r0 udiv16_divisor  ; remainder -= divisor
        tpsl    PSL_C
        bctr,eq udiv16_lo_one   ; branch if remainder >= divisor (PSL.C=1)
        rrl     r2              ; quotient <<= 1
        cpsl    PSL_C           ; PSL.C=0
        addr,r1 udiv16_divisor+1
        addr,r0 udiv16_divisor  ; remainder += divisor
        bdrr,r3 udiv16_lo_loop
        bctr,un udiv16_lo_end
udiv16_lo_one:
        rrl     r2              ; quotient = quotient << 1 | 1
        bdrr,r3 udiv16_lo_loop
udiv16_lo_end:
        loda,r3 udiv16_bits
        bdrr,r3 udiv16_back     ; R3=bits-1
udiv16_back_loop:
        cpsl    PSL_C           ; PSL.C=0
        rrr     r0
        rrr     r1              ; remainder >>= 1
udiv16_back:
        bdrr,r3 udiv16_back_loop ; bits-2 times
udiv16_store:
        stra,r0 udiv16_reminder
        stra,r1 udiv16_reminder+1
        lodz    r2
        strz    r1
        loda,r0 udiv16_quotient ; R0:R1=quotient
        loda,r2 udiv16_r2
        loda,r3 udiv16_r3
        retc,un
udiv16_hi_loop:
        cpsl    PSL_C           ; PSL.C=0
        rrl     r1
        rrl     r0              ; remainder <<= 1
udiv16_hi_enter:
        ppsl    PSL_C           ; PSL.C=1
        suba,r1 udiv16_divisor+1
        suba,r0 udiv16_divisor  ; remainder -= divisor
        tpsl    PSL_C
        bctr,eq udiv16_hi_one   ; branch if remainder >= divisor (PSL.C=1)
        rrl     r2              ; quotient <<= 1
        cpsl    PSL_C           ; PSL.C=0
        adda,r1 udiv16_divisor+1
        adda,r0 udiv16_divisor  ; remainder += divisor
        bdrr,r3 udiv16_hi_loop
        bctr,un udiv16_hi_end
udiv16_hi_one:
        rrl     r2              ; quotient = quotient << 1 | 1
        bdrr,r3 udiv16_hi_loop
udiv16_hi_end:
        stra,r2 udiv16_quotient
        lodi,r3 8
        bcta,un udiv16_lo_loop

div16_hi:
        res     1
div16_sign:
        res     1

;;; Signed division: quotient = dividend / divisor
;;; @param R0:R1 dividend
;;; @param R2:R3 divisor
;;; @return R0:R1 quotient
;;; @return R2:R3 remainder of the absolute values
;;; @clobber PSL
div16:
        brna,r2 div16_wide      ; branch if divisor < 0 or divisor >= 256
        bdrr,r3 div16_short     ; R3=divisor-1
div16_short:
        comi,r3 H'7F'
        bcta,gt div16_wide_r3   ; branch if divisor == 0 or divisor > 128
        strr,r0 div16_sign      ; sign of quotient is the dividend's
        iorz    r0
        bcfr,lt div16_abs       ; branch if dividend >= 0
        eori,r1 H'FF'
        cpsl    PSL_C           ; PSL.C=0
        addi,r1 1
        eori,r0 H'FF'
        addi,r0 0               ; dividend=-dividend
div16_abs:
        bcta,eq div16_lo        ; branch if hi(dividend) == 0
        comz    r3
        bcta,gt div16_wide_abs  ; branch if hi(dividend) >= divisor
;;; 16/8 divide: R0 holds the remainder while R1 shifts the dividend
;;; out and the quotient in through PSL.C. A remainder below 128
;;; never carries out of R0 when shifted.
div16_byte:
        rrl     r1
        rrl     r0              ; remainder <<= 1, PSL.C=0
        comz    r3
        bcfr,gt div16_b0        ; branch if remainder < divisor
        subz    r3              ; remainder -= divisor, PSL.C=1
div16_b0:
        rrl     r1              ; quotient bit into R1
        rrl     r0
        comz    r3
        bcfr,gt div16_b1
        subz    r3
div16_b1:
        rrl     r1
        rrl     r0
        comz    r3
        bcfr,gt div16_b2
        subz    r3
div16_b2:
        rrl     r1
        rrl     r0
        comz    r3
        bcfr,gt div16_b3
        subz    r3
div16_b3:
        rrl     r1
        rrl     r0
        comz    r3
        bcfr,gt div16_b4
        subz    r3
div16_b4:
        rrl     r1
        rrl     r0
        comz    r3
        bcfr,gt div16_b5
        subz    r3
div16_b5:
        rrl     r1
        rrl     r0
        comz    r3
        bcfr,gt div16_b6
        subz    r3
div16_b6:
        rrl     r1
        rrl     r0
        comz    r3
        bcfr,gt div16_b7
        subz    r3
div16_b7:
        rrl     r1              ; R1=quotient
        strz    r3              ; R3=remainder
        loda,r2 div16_sign
        bctr,lt div16_minus
        eorz    r0
        strz    r2              ; R0=hi(quotient)=0, R2=hi(remainder)=0
        retc,un
div16_minus:
        eorz    r0
        strz    r2
        bcta,un negsi1          ; quotient=-quotient
div16_lo:
        lodz    r1
        comz    r3
        bcfr,gt div16_zero      ; branch if dividend < divisor
        eorz    r0
        comi,r3 31
        bcta,lt div16_byte      ; branch if divisor < 32
;;; A divisor of 32 or more leaves a quotient below 8 here, so the
;;; first 5 quotient bits are 0
        rrl     r1
        rrl     r0
        rrl     r1
        rrl     r0
        rrl     r1
        rrl     r0
        rrl     r1
        rrl     r0
        rrl     r1
        rrl     r0              ; remainder=dividend>>3
        bcta,un div16_b4
div16_zero:
        strz    r3              ; remainder=dividend
        eorz    r0
        strz    r1              ; quotient=0
        retc,un
div16_wide_abs:
        birr,r3 div16_wide_u    ; R3=divisor
div16_wide_r3:
        birr,r3 div16_wide      ; R3=divisor
div16_wide:
        stra,r0 div16_hi
        eorz    r2
        stra,r0 div16_sign      ; sign of quotient
        loda,r0 div16_hi
        bsta,lt negsi1          ; dividend=-dividend
div16_wide_u:
        stra,r0 udiv16_dividend
        lodz    r1
        stra,r0 udiv16_dividend+1
        lodz    r3
        strz    r1
        lodz    r2
        bsta,lt negsi1          ; divisor=-divisor
        stra,r0 udiv16_divisor
        lodz    r1
        stra,r0 udiv16_divisor+1
        bsta,un udiv16          ; R0:R1=quotient
        loda,r2 div16_sign
        bsta,lt negsi1          ; quotient=-quotient
        loda,r2 udiv16_reminder
        loda,r3 udiv16_reminder+1
        retc,un

;;; Signed division: dividend /= divisor
;;; @param @R2: dividend
;;; @param @R3: divisor
;;; @return udiv16_reminder: remainder
;;; @clobber R0 R1 PSL
divsi2:
        strr,r2 divsi2_r2
        strr,r3 divsi2_r3
        loda,r0 arith_work,r3
        strr,r0 divsi2_hi
        loda,r0 arith_work+1,r3
        strz    r3
        loda,r0 arith_work+1,r2
        strz    r1
        loda,r0 arith_work,r2
        lodr,r2 divsi2_hi       ; R0:R1=dividend, R2:R3=divisor
        bsta,un div16
        stra,r2 udiv16_reminder
        stra,r3 udiv16_reminder+1
        lodr,r2 divsi2_r2
        lodr,r3 divsi2_r3
        stra,r0 arith_work,r2
        lodz    r1
        stra,r0 arith_work+1,r2
        retc,un
divsi2_hi:
        res     1
divsi2_r2:
        res     1
divsi2_r3:
        res     1
