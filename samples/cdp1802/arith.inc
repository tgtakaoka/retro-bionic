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
        stxd                    ; push remainder
        sep     R5
        dc      A(print_uint16_loop)
        inc     R2
        ldn     R2              ; pop remainder
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
;;; @clobber D R15
store_R7:
        sex     R4
        ldxa
        phi     R15
        ldxa
        plo     R15             ; R15=&valiable
        ghi     R7
        str     R15
        inc     R15
        glo     R7
        str     R15
        sex     R2
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

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram R7 dividend
;;; @praram R8 divisor
;;; @return R7 quotient
;;; @return R8 remainder
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
        ldi     1               ; 1=no borrow
        phi     R15             ; R15.1=borrow
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
        ghi     R15             ; borrow
        bz      udiv16_add      ; brancg if borrow (0)
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
        br      udiv16_quotient
udiv16_add:
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
udiv16_quotient:
        ldi     0
        shlc                    ; LSB(D)=DF; 0 if borrow
        phi     R15             ; borrow
        bz      udiv16_next
        inc     R7              ; quotient += 1
udiv16_next:
        dec     R15
        glo     R15
        bnz     udiv16_loop     ; while bits != 0
        ghi     R15             ; borrow
        bnz     udiv16_return   ; branch if no borrow (1)
        glo     R8              ; adjust dividend
        str     R2
        glo     R9
        add
        plo     R9
        ghi     R8
        str     R2
        ghi     R9
        adc
        phi     R9              ; dividend+=divisor
udiv16_return:
        ghi     R9
        phi     R8
        glo     R9
        plo     R8              ; R8=remainder
        irx
        ldxa
        phi     R9
        ldx
        plo     R9              ; restore R9
        sep     R6              ; return

;;; Signed division: dividend / divisor = quotient ... remainder
;;; @param R7 dividend
;;; @param R8 divisor
;;; @return R7 quotient
;;; @return R8 remainder
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
