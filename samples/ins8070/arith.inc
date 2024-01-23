;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print signed 16-bit integer as decimal
;;; @param EA: value
print_int16:
        push    P3
        push    EA
        ld      P3, EA          ; P3=value
        ld      A, E            ; A=high(value)
        bp      print_int16_plus
        ld      A, ='-'
        jsr     putchar         ; print '-'
        ld      EA, =0
        sub     EA, 0, SP       ; negate value
        ld      P3, EA          ; P3=|value|
print_int16_plus:
        jsr     print_uint16
        pop     EA
        pop     P3
        ret
;;; Print unsigned 16-bit integer
;;; @param P3 value
print_uint16:
        ld      EA, P3
        or      A, E
        bz      print_uint16_zero
print_uint16_loop:
        ld      EA, P3
        or      A, E
        bz      print_uint16_end
        jsr     divmod10        ; P3=value/10, EA=value%10
        push    A               ; push reminder
        jsr     print_uint16_loop
        pop     A
print_uint16_zero:
        or      A, ='0'
        jsr     putchar
print_uint16_end:
        ret

;;; Divide by 10
;;; @param P3 value
;;; @return P3 value/10
;;; @return EA value%10
divmod10:
        ld      EA, P3
        ld      T, =10
        div     EA, T
        push    EA              ; save value/10
        ld      T, =10
        mpy     EA, T           ; T=(value/10)*10
        ld      EA, T
        push    EA              ; local variable
        ld      EA, P3          ; EA=value
        sub     EA, 0, SP       ; EA=value-(value/10)*10
        pop     P3              ; discard local
        pop     P3              ; restore value/10
        ret

;;; Signed comparison: minuend - subtrahend
;;; @param EA minuend
;;; @param T  subtrahend
;;; @return A=0  BZ (minuend == subtrahend)
;;;         A=1  BP (minuend > subtrahend)
;;;         A=-1    (minuend < subtrahend)
;;; @clobber EA
cmpsi2:
        push    EA              ; S[1:0]=minuend
        ld      EA, T           ; EA=subtrahend
        sub     EA, 0, SP       ; EA=subtrahend-minuend
        st      EA, 0, SP
        or      A, E
        bz      cmpsi2_equal    ; branch if A=0
        ld      A, 1, SP
        bp      cmpsi2_plus     ; branch if A>0 (N=0)
cmpsi2_minus:                   ; A<0 (N=1)
        ld      A, S
        and     A, =S_OV
        bnz     cmpsi2_less     ; branch if V=1
        ;; V=0, N=1
cmpsi2_great:                   ; N^V=1
        pop     EA
        ld      A, =1
        ret
cmpsi2_plus:                    ; N=0
        ld      A, S
        and     A, =S_OV
        bnz     cmpsi2_great    ; branch if V=1, N=0
        ;; V=0, N=0
cmpsi2_less:                    ; N^V=0
        pop     EA
        ld      A, =-1
        ret
cmpsi2_equal:
        pop     EA
        ld      A, =0
        ret

;;; Signed multiplication: product = multiplicand * multiplier
;;; @param EA: multiplicand
;;; @param T: multiplier
;;; @return EA: product
;;; @local SP[4:3] multiplicand
;;; @local SP[2:1] multiplier
;;; @local SP[0] sign(product)
;;; @discard T
mulsi2:
        push    EA             ; save multiplicand
        ld      EA, T          ; EA=multiplier
        push    EA             ; save multiplier
        ld      A, E           ; A=high(multiplier)
        xor     A, 3, SP       ; A=sign(product)
        push    A              ; save sign(product)
        ld      A, 2, SP       ; A=high(multiplier)
        bp      mulsi2_multiplicand
        ld      EA, =0
        sub     EA, 1, SP
        st      EA, 1, SP       ; negate multiplier
mulsi2_multiplicand:
        ld      A, 4, SP        ; A=high(multiplicand)
        bp      mulsi2_multiply
        ld      EA, =0
        sub     EA, 3, SP
        st      EA, 3, SP       ; negate multiplicand
mulsi2_multiply:
        ld      EA, 3, SP       ; EA=|multiplicand|
        ld      T, 1, SP        ; T=|multiplier|
        mpy     EA, T           ; T=product
        ld      EA, T
        st      EA, 3, SP       ; store product
        pop     A               ; A=high(multiplicand^multiplier)
        bp      mulsi2_exit
        ld      EA, =0
        sub     EA, 2, SP       ; negate product
        st      EA, 2, SP       ; store product
mulsi2_exit:
        pop     EA
        ld      T, EA           ; T=multiplier
        pop     EA              ; EA=product
        ret

;;; Signed division: quotient = dividend * divisor
;;; @param EA: dividend
;;; @param T: divisor
;;; @return EA: quotient
;;; @discard T
;;; @local SP[4:3] dividend
;;; @local SP[2:1] divisor
;;; @local SP[0] sign(quotient)
divsi2:
        push    EA              ; save dividend
        ld      EA, T           ; EA=divisor
        push    EA              ; save divisor
        ld      A, E            ; A=high(divisor)
        xor     A, 3, SP        ; A=sign(quotient)
        push    A               ; save sign(quotient)
        ld      A, 2, SP        ; A=high(divisor)
        bp      divsi2_dividend
        ld      EA, =0
        sub     EA, 1, SP
        st      EA, 1, SP       ; negate divisor
divsi2_dividend:
        ld      A, 4, SP        ; A=high(dividend)
        bp      divsi2_divide
        ld      EA, =0
        sub     EA, 3, SP
        st      EA, 3, SP       ; negate dividend
divsi2_divide:
        ld      EA, 3, SP       ; EA=|dividend|
        ld      T, 1, SP        ; T=|divisor|
        div     EA, T           ; EA=quotient
        st      EA, 3, SP       ; store quotient
        pop     A               ; A=sign(quotient)
        bp      divsi2_exit
        ld      EA, =0
        sub     EA, 2, SP       ; negate quotient
        st      EA, 2, SP       ; store quotient
divsi2_exit:
        pop     EA
        ld      T, EA           ; T=divisor
        pop     EA              ; EA=quotient
        ret
