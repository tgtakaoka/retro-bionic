;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     1802

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
        sep     R6              ; return if value=0
print_uint16_digit:
        ldi     0
        phi     R8
        ldi     10
        plo     R8              ; divisor=10
        sep     R5
        dc      A(udiv16)       ; call udiv16
        glo     R8
        stxd                    ; push reminder
        sep     R5
        dc      A(print_uint16_loop)
        inc     R2
        ldn     R2              ; pop reminder
print_uint16_zero:
        adi     T'0'
        br      putchar

;;; Print signed 16-bit integer as decimal
;;; @param R7 value
;;; @clobber D R15
print_int16:
        glo     R8
        stxd
        ghi     R8
        stxd                    ; save R8
        glo     R7
        stxd
        ghi     R7
        stxd                    ; save R7
        ani     X'80'
        bz      print_int16_print
        ldi     T'-'
        sep     R5
        dc      A(putchar)      ; print '-'
        ghi     R7
        xri     X'FF'
        phi     R7
        glo     R7
        xri     X'FF'
        plo     R7
        inc     R7              ; negate value
print_int16_print:
        sep     R5
        dc      A(print_uint16)
        irx
        ldxa
        phi     R7
        ldxa
        plo     R7              ; restore R7
        ldxa
        phi     R8
        ldx
        plo     R8              ; restore R8
        sep     R6

;;; Store R7 to variable
;;;   SEP  R5
;;;   DC   A(store_R7)
;;;   DC   A(variable)
;;; @clobber D
store_R7:
        glo     R8
        stxd
        ghi     R8
        stxd                    ; save R8
        sex     R4
        ldxa
        phi     R8
        ldxa
        plo     R8              ; R8=&valiable
        ghi     R7
        str     R8
        inc     R8
        glo     R7
        str     R8
        sex     R2
        irx
        ldxa
        phi     R8
        ldx
        plo     R8              ; restore R8
        sep     R6

;;; Load variable to R7
;;;   SEP  R5
;;;   DC   A(load_R7)
;;;   DC   A(variable)
;;; @return R7 variable
;;; @clobber D R15
load_R7:
        sex     R4
        ldxa
        phi     R7
        ldxa
        plo     R7
        ldn     R7
        plo     R15
        inc     R7
        ldn     R7
        plo     R7
        glo     R15
        phi     R7
        sex     R2
        sep     R6

;;; Load variable to R8
;;;   SEP  R5
;;;   DC   A(load_R8)
;;;   DC   A(variable)
;;; @clobber R15
load_R8:
        sex     R4
        ldxa
        phi     R8
        ldxa
        plo     R8
        ldn     R8
        plo     R15
        inc     R8
        ldn     R8
        plo     R8
        glo     R15
        phi     R8
        sex     R2
        sep     R6

;;; Set constant to R7
;;;   SEP  R5
;;;   DC   A(set_R7)
;;;   DC   A(constant)
;;; @return R7 constant
;;; @clobber D
set_R7:
        sex     R4
        ldxa
        phi     R7
        ldxa
        plo     R7
        sex     R2
        sep     R6

;;; Set constant to R8
;;;   SEP  R5
;;;   DC   A(set_R8)
;;;   DC   A(variable)
;;; @clobber R15
set_R8:
        sex     R4
        ldxa
        phi     R8
        ldxa
        plo     R8
        sex     R2
        sep     R6

;;; Increment variable
;;;   SEP  R5
;;;   DC   A(inc16)
;;;   DC   A(variable)
;;; @return R7 variable
;;; @clobber D R15
inc16:
        glo     R8
        stxd
        ghi     R8
        stxd                    ; save R8
        sex     R4
        ldxa
        phi     R8
        ldxa
        plo     R8
        ldn     R8
        phi     R7
        inc     R8
        ldn     R8
        plo     R7
        inc     R7
        glo     R7
        str     R8
        ghi     R7
        dec     R8
        str     R8
        sex     R2
        irx
        ldxa
        phi     R8
        ldx
        plo     R8              ; restore R8
        sep     R6

;;; Signed addition: summand += addend
;;; @param R7 summand
;;; @param R8 addend
;;; @return R7 summand + addend
;;;   SEP R5
;;;   DC  A(add16)
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
        sep     R6

;;; Singed subtraction: minuend -= subtrahend
;;; @param R7 minuend
;;; @param R8 subtrahend
;;; @return R7 minuend - subtrahend
;;;   SEP R5
;;;   DC  A(sub16)
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
        sep     R6

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
        sep     R6              ; return with D=0, DF=1
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
        sep     R6              ; return with D=1

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
        sep     R6              ; return

;;; Signed multiplication: multiplicand *= multiplier
;;; @param R7 multiplicand
;;; @param R8 multiplier
;;; @return R7 multiplicand * multiplier
;;;   SEP R5
;;;   DC  A(mul16)
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
        sep     R5
        dc      A(umul16)       ; call umul16
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
        sep     R6

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
        sep     R6              ; return divide by zero
udiv16_calc:
        glo     R9
        stxd
        ghi     R9
        stxd                    ; save R9
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
        ldxa
        phi     R9
        ldx
        plo     R9              ; restore R9
        sep     R6              ; return

;;; Signed division: dividend / divisor = quotient ... reminder
;;; @param R7 dividend
;;; @param R8 divisor
;;; @return R7 quotient
;;; @return R8 reminder
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
        sep     R5
        dc      A(udiv16)       ; call udiv16
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
        sep     R6
