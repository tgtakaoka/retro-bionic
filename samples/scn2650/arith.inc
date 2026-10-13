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

;;; Signed multiplication: multiplicand *= multiplier
;;; @param @R2: multiplicand
;;; @param @R3: multiplier
;;; @clobber R0 R1 PSL
mulsi2:
        loda,r0 arith_work,r3
        iora,r0 arith_work+1,r3
        bcfr,eq mulsi2_signed   ; branch if multiplier != 0
        stra,r0 arith_work,r2
        stra,r0 arith_work+1,r2 ; multiplicand=0
        retc,un
mulsi2_signed:
        loda,r0 arith_work,r2
        eora,r0 arith_work,r3
        strr,r0 mulsi2_sign
        loda,r0 arith_work+1,r2
        strz    r1
        loda,r0 arith_work,r2
        bsta,lt negsi1          ; multiplicand=-multiplicand
        strr,r0 umul16_multiplicand
        lodz    r1
        strr,r0 umul16_multiplicand+1
        loda,r0 arith_work+1,r3
        strz    r1
        loda,r0 arith_work,r3
        bsta,lt negsi1          ; multiplier=-multiplier
        stra,r0 umul16_multiplier
        lodz    r1
        stra,r0 umul16_multiplier+1
        bsta,un umul16          ; r0:r1=multiplicand*multiplier
        strr,r2 mulsi2_r2
        lodr,r2 mulsi2_sign
        bsta,lt negsi1          ; result=-result
        lodr,r2 mulsi2_r2
        stra,r0 arith_work,r2
        lodz    r1
        stra,r0 arith_work+1,r2
        retc,un
mulsi2_sign:
        res     1
mulsi2_r2:
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

;;; Signed division: dividend /= divisor
;;; @param @R2: dividend
;;; @param @R3: divisor
;;; @return udiv16_reminder: remainder
;;; @clobber R0 R1 PSL
divsi2:
        loda,r0 arith_work,r2
        eora,r0 arith_work,r3
        strr,r0 divsi2_sign
        loda,r0 arith_work+1,r2
        strz    r1
        loda,r0 arith_work,r2
        bsta,lt negsi1          ; dividend=-dividend
        stra,r0 udiv16_dividend
        lodz    r1
        stra,r0 udiv16_dividend+1
        loda,r0 arith_work+1,r3
        strz    r1
        loda,r0 arith_work,r3
        bsta,lt negsi1          ; divisor=-divisor
        stra,r0 udiv16_divisor
        lodz    r1
        stra,r0 udiv16_divisor+1
        bsta,un udiv16          ; r0:r1=quotient
        strr,r2 divsi2_r2
        lodr,r2 divsi2_sign
        bsta,lt negsi1          ; quotient=-quotient
        lodr,r2 divsi2_r2
        stra,r0 arith_work,r2
        lodz    r1
        stra,r0 arith_work+1,r2
        retc,un
divsi2_sign:
        res     1
divsi2_r2:
        res     1
