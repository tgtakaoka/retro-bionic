;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     1804A

;;; Print unsigned 16-bit integer as decimal
;;; @param R7 value
;;; @clobber D R7 R8 R15
print_uint16:
        ghi     R7
        bnz     print_uint16_loop
        glo     R7
        bz      print_uint16_zero
print_uint16_loop:
        ghi     R7
        bnz     print_uint16_digit
        glo     R7
        bnz     print_uint16_digit
        sret    R4
print_uint16_digit:
        ldi     0
        phi     R8
        ldi     10
        plo     R8              ; divisor=10
        scal    R4, udiv16
        glo     R8
        stxd                    ; push reminder
        scal    R4, print_uint16_loop
        inc     R2
        ldn     R2              ; pop reminder
print_uint16_zero:
        adi     T'0'
        br      putchar

;;; Print signed 16-bit integer as decimal
;;; @param R7 value
;;; @clobber D R15
print_int16:
        rsxd    R8              ; save R8
        rsxd    R7              ; save R7
        ghi     R7
        ani     X'80'
        bz      print_int16_print
        ldi     T'-'
        scal    R4, putchar      ; print '-'
        ghi     R7
        xri     X'FF'
        phi     R7
        glo     R7
        xri     X'FF'
        plo     R7
        inc     R7              ; negate value
print_int16_print:
        scal    R4, print_uint16
        irx
        rlxa    R7              ; restore R7
        rlxa    R8              ; restore R8
        dec     R2
        sret    R4

;;; Store R7 to variable
;;;   SCAL R4, store_R7
;;;   DC   A(variable)
;;; @clobber D
store_R7:
        rsxd    R8              ; save R8
        sex     R4
        rlxa    R8              ; R8=&valiable
        sex     R8
        irx
        rsxd    R7
        sex     R2
        irx
        rlxa    R8              ; restore R8
        dec     R2
        sret    R4

;;; Load variable to R7
;;;   SCAL R4, load_R7
;;;   DC   A(variable)
;;; @return R7 variable
;;; @clobber R15
load_R7:
        sex     R4
        rlxa    R7              ; R7=&variable
        sex     R7
        rlxa    R7
        sex     R2
        sret    R4

;;; Load variable to R8
;;;   SCAL R4, load_R8
;;;   DC   A(variable)
;;; @clobber R15
load_R8:
        sex     R4
        rlxa    R8              ; R8=&variable
        sex     R8
        rlxa    R8
        sex     R2
        sret    R4

;;; Increment variable
;;;   SCAL R4, inc16
;;;   DC   A(variable)
;;; @return R7 variable
inc16:
        rsxd    R8              ; save R8
        sex     R4
        rlxa    R8              ; R8=$variable
        sex     R8
        rlxa    R7              ; load R7
        inc     R7
        dec     R8
        rsxd    R7              ; save R7
        sex     R2
        irx
        rlxa    R8              ; restore R8
        dec     R2
        sret    R4

;;; Signed addition: summand += addend
;;; @param R7 summand
;;; @param R8 addend
;;; @return R7 summand + addend
;;;   SCAL R4, add16
;;; @clobber D
add16:
        glo     R8
        str     R2
        glo     R7
        add
        plo     R7
        ghi     R8
        str     R2
        ghi     R7
        adc
        phi     R7
        sret    R4

;;; Singed subtraction: minuend -= subtrahend
;;;   SCAL R4, sub16
;;; @param R7 minuend
;;; @param R8 subtrahend
;;; @return R7 minuend - subtrahend
;;; @clobber D
sub16:
        glo     R8
        str     R2
        glo     R7
        sm
        plo     R7
        ghi     R8
        str     R2
        ghi     R7
        smb
        phi     R7
        sret    R4

;;; Signed comparison: minuend - subtrahend
;;; @param R7 minuend
;;; @param R8 subtrahend
;;; @return D=0 DF=1 (minuend==subtrahend); BZ
;;;         D=1 DF=1 (minuend>subtrahend);  BGE
;;;         D=1 DF=0 (minuend<subtrahend);  BL
;;; @clobber R7 R8
;;; result = minuend - subtrahend
;;; Z=(result.1 | result.0) == 0
;;; N=(result.1 & 0x80) != 0
;;; V=((minuend.1 ^ subtrahend.1) & (result.1 ^ minuend.1) & 0x80) != 0
;;; LT=N ^ V
cmp16:
        glo     R8              ; D=subtrahend.0
        str     R2              ; stack top=subtrahend.0
        glo     R7              ; D=minuend.0
        sm                      ; D=minuend.0=subtrahend.0
        plo     R7              ; R7.0=result.0
        ghi     R8              ; D=subtrahend.1
        str     R2              ; stack top=subtrahend.1
        ghi     R7              ; D=minuend.1
        xor                     ; D=minuend.1^subtrahend.1
        plo     R8              ; R8.0=minuend.1^subtrahend.1
        ghi     R7              ; D=minuend.1
        smb                     ; D=minuend.1=subtrahend.1
        phi     R8              ; R8.1=result.1
        bnz     cmp16_neq       ; branch if result.1!=0
        glo     R7              ; D=result.0
        bnz     cmp16_neq       ; branch if result.0!=-
        ldi     1
        shr
        sret    R4
cmp16_neq:
        ghi     R8              ; D=result.1
        str     R2
        ghi     R7              ; D=minuend.1
        xor                     ; D=result.1^minuend.1
        str     R2              ; stack top=result.1^minuend.1
        glo     R8              ; D=minuend.1^subtrahend.1
        and                     ; D=(minuend.1^subtrahend.1)&(result.1^minuend.1)
        str     R2              ; stack top=V
        ghi     R8              ; D=result.1
        xor                     ; D=N^V
        xri     X'80'           ; D=~(N^V)
        shl                     ; DF=~(N^V)
        ldi     1
        sret    R4

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param R7 multiplicand
;;; @param R8 multiplier
;;; @return R7 result
;;; @clobber D R7 R8 R15
umul16:
        ldi     0
        phi     R15
        plo     R15             ; R15=result
        br      umul16_check
umul16_loop:
        glo     R8
        ani     1
        bz      umul16_sr       ; lsb(multiplier)==0
        glo     R7
        str     R2              ; stack top=multiplicand.0
        glo     R15
        add
        plo     R15
        ghi     R7
        str     R2              ; stack top=multiplicand.1
        ghi     R15
        adc
        phi     R15             ; result += multiplicand
umul16_sr:
        ghi     R8
        shr
        phi     R8
        glo     R8
        shrc
        plo     R8              ; multiplier >>= 1
        glo     R7
        shl
        plo     R7
        ghi     R7
        shlc
        phi     R7              ; multiplicand <<= 1
umul16_check:
        ghi     R8
        bnz     umul16_loop     ; while multiplier != 0
        glo     R8
        bnz     umul16_loop     ; while multiplier != 0
        ghi     R15
        phi     R7
        glo     R15
        plo     R7              ; R7=result
        sret    R4

;;; Signed multiplication: multiplicand *= multiplier
;;; @param R7 multiplicand
;;; @param R8 multiplier
;;; @return R7 multiplicand * multiplier
;;;   SCAL R4, mul16
;;; @clobber R8 R15
mul16:
        ghi     R8
        str     R2
        ghi     R7
        xor
        stxd                    ; push sign
        ghi     R8
        ani     X'80'
        bz      mul16_multiplicand
        ghi     R8
        xri     X'FF'
        phi     R8
        glo     R8
        xri     X'FF'
        plo     R8
        inc     R8              ; negate multiplier
mul16_multiplicand:
        ghi     R7
        ani     X'80'
        bz      mul16_multiply
        ghi     R7
        xri     X'FF'
        phi     R7
        glo     R7
        xri     X'FF'
        plo     R7
        inc     R7              ; negate multiplicand
mul16_multiply:
        scal    R4, umul16
        irx
        ldx                     ; sign
        ani     X'80'
        bz      mul16_return
        ghi     R7
        xri     X'FF'
        phi     R7
        glo     R7
        xri     X'FF'
        plo     R7
        inc     R7              ; negate result
mul16_return:
        sret    R4

;;; Unsigned division: dividend / divisor = quotient ... reminder
;;; @praram R7 dividend
;;; @praram R8 divisor
;;; @return R7 quotient
;;; @return R8 reminder
;;; @clobber R7 R8 R15
udiv16:
        ghi     R8
        bnz     udiv16_calc
        glo     R8
        bnz     udiv16_calc
        sret    R4
udiv16_calc:
        rsxd    R9              ; save R9
        ldi     1
        plo     R15             ; R15.0=bits
        br      udiv16_prep
udiv16_prep_loop:
        glo     R8
        shl
        plo     R8
        ghi     R8
        shlc
        phi     R8              ; divisor <<= 1
        inc     R15             ; ++bits
udiv16_prep:
        ghi     R8
        ani     X'80'
        bz      udiv16_prep_loop ; while msb(divisor) == 0
        ghi     R7
        phi     R9
        glo     R7
        plo     R9              ; R9=dividend
        ldi     0
        phi     R7
        plo     R7              ; R7=quotient
        br      udiv16_enter_loop
udiv16_loop:
        ghi     R8
        shr
        phi     R8
        glo     R8
        shrc
        plo     R8              ; divisor >>= 1
        glo     R7
        shl
        plo     R7
        ghi     R7
        shlc
        phi     R7              ; quotient <<= 1
udiv16_enter_loop:
        glo     R8
        str     R2
        glo     R9
        sm
        plo     R9
        ghi     R8
        str     R2
        ghi     R9
        smb
        phi     R9              ; dividend-=divisor
        bm      udiv16_readd    ; branch if dividend < 0
        inc     R7              ; quotient += 1
        br      udiv16_next
udiv16_readd:
        glo     R8
        str     R2
        glo     R9
        add
        plo     R9
        ghi     R8
        str     R2
        ghi     R9
        adc
        phi     R9              ; dividend+=divisor
udiv16_next:
        dec     R15
        glo     R15
        bnz     udiv16_loop     ; while bits != 0
        ghi     R9
        phi     R8
        glo     R9
        plo     R8              ; R8=reminder
        irx
        rlxa    R9              ; restore R9
        dec     R2
        sret    R4

;;; Signed division: dividend / divisor = quotient ... reminder
;;; @param R7 dividend
;;; @param R8 divisor
;;; @return R7 quotient
;;; @return R8 reminder
;;;   SCAL R4, duvsi2
;;;   SEP R5
;;;   DC  A(div16)
;;; @clobber R15
div16:
        ghi     R8
        str     R2
        ghi     R7
        xor
        stxd                    ; push sign
        ghi     R8
        ani     X'80'
        bz      div16_dividend
        ghi     R8
        xri     X'FF'
        phi     R8
        glo     R8
        xri     X'FF'
        plo     R8
        inc     R8              ; negate divisor
div16_dividend:
        ghi     R7              ; R7=dividend
        ani     X'80'
        bz      div16_divide
        ghi     R7
        xri     X'FF'
        phi     R7
        glo     R7
        xri     X'FF'
        plo     R7
        inc     R7              ; negate dividend
div16_divide:
        scal    R4, udiv16
        irx
        ldx                     ; pop sign
        ani     X'80'
        bz      div16_return
        ghi     R7
        xri     X'FF'
        phi     R7
        glo     R7
        xri     X'FF'
        plo     R7
        inc     R7              ; negate quotient
div16_return:
        sret    R4
