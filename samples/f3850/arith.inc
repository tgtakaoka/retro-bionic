;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     f3850

;;; Print unsigned 16-bit integer as decimal
;;; @param 0:1 value
;;; @clobber 0 1 4 5 6 7 A
print_uint16:
        lr      A, 0
        oi      0
        bnz     print_uint16_inner
        lr      A, 1
        oi      0
        bz      print_uint16_zero
print_uint16_inner:
        lr      A, 0
        lr      4, A
        lr      A, 1
        lr      5, A            ; 4:5=value
print_uint16_loop:
        lr      A, 4
        oi      0
        bnz     print_uint16_digit
        lr      A, 5
        oi      0
        bnz     print_uint16_digit
        jmp     return
print_uint16_digit:
        clr
        lr      6, A
        li      10
        lr      7, A            ; 6:7=10
        pi      call
        da      udiv16          ; 4:5/6:7=4:5...6:7
        lr      A, 7
        lr      0, A
        pi      push0           ; push remainder
        pi      call
        da      print_uint16_loop
        pi      pop0
print_uint16_zero:
        li      C'0'
        as      0
        lr      0, A
        jmp     putchar

;;; Print signed 16-bit integer as decimal
;;; @param 0:1 value
;;; @clobber 0 1 4 5 6 7 A
print_int16:
        lr      A, 0
        oi      0
        bp      print_uint16
        pi      push0
        li      C'-'
        lr      0, A
        pi      call
        da      putchar         ; print '-'
        pi      pop0
        lr      A, 0
        com
        lr      0, A
        lr      A, 1
        com
        inc
        lr      1, A
        lr      A, 0
        lnk
        lr      0, A            ; 0:1=-value
        br      print_uint16

;;; Negation; result = -value
;;; @param @2: result
;;; @param @3: value
;;; @clobber 0 1 ISAR A
negsi2:
        lr      A, 3
        lr      IS, A           ; point MSB(value)
        lr      A, I
        com
        lr      0, A            ; MSB(~value)
        lr      A, S
        com
        lr      1, A            ; LSB(~value)
        lr      A, 2
        inc
        lr      IS, A           ; point LSB(@2)
        lr      A, 1
        inc
        lr      D, A            ; LSB(-value)
        lr      A, 0
        lnk
        lr      S, A            ; MSB(-value)
        jmp     return

;;; Negation; result = -result
;;; @param 0:1 result
;;; @clobber A
negsi1:
        lr      A, 0
        com
        lr      0, A            ; MSB(~result)
        lr      A, 1
        com
        inc
        lr      1, A            ; LSB(-result)
        lr      A, 0
        lnk
        lr      0, A            ; MSB(-result)
        jmp     return

;;; Signed addition: summand += addend
;;; @param @2: summand
;;; @param @3: addend
;;; @clobber 0 1 ISAR A
addsi2:
        lr      A, 3
        lr      IS, A           ; point MSB(addend)
        lr      A, I
        lr      0, A            ; MSB(addend)
        lr      A, S
        lr      1, A            ; LSB(addend)
        lr      A, 2
        inc
        lr      IS, A           ; point LSB(summand)
        lr      A, S
        as      1
        lr      D, A            ; store LSB(summand)
        lr      A, S
        lnk
        as      0
        lr      S, A
        jmp     return

;;; Singed subtraction: minuend -= subtrahend
;;; @param @2: minuend
;;; @param @3: subtrahend
;;; @clobber 0 1 A
subsi2:
        lr      A, 3
        lr      IS, A           ; point MSB(subtrand)
        lr      A, I
        com
        lr      0, A            ; MSB(~subtrand)
        lr      A, S
        com
        inc
        lr      1, A            ; LSB(-subtrand)
        lr      A, 0
        lnk
        lr      0, A            ; MSB(-subtrand)
        lr      A, 2
        inc
        lr      IS, A           ; point LSB(minuend)
        lr      A, S
        as      1
        lr      D, A            ; store LSB(minuend)
        lr      A, S
        lnk
        as      0
        lr      S, A
        jmp     return

;;; Signed comparison: minuend - subtrahend
;;; @param @2: minuend
;;; @param @3: subtrahend
;;; @return W.Z, W.S
;;; @clobber 0 1 ISAR A J
cmpsi2:
        lr      A, 3
        lr      IS, A           ; point MSB(subtrahend)
        lr      A, I
        com
        lr      0, A            ; MSB(~subtrahend)
        lr      A, S
        com
        inc
        lr      1, A            ; LSB(-subtrahend)
        lr      A, 0
        lnk
        lr      0, A            ; MSB(-subtrahend)
        lr      A, 2
        inc
        lr      IS, A           ; point LSB(minuend)
        lr      A, D
        as      1
        lr      1, A            ; LSB(minued-subtrahend)
        lr      A, S
        lnk
        as      0
        lr      0, A            ; MSB(minued-subtrahend)
        clr                     ; MSB(A)=overflow
        bno     cmpsi2_nov      ; no overflow
        xi      H'80'           ; MSB(A)=overflow
cmpsi2_nov:
        lr      J, A            ; MSB(J)=overflow
        lr      A, 0
        oi      0
        bnz     cmpsi2_cmp
        lr      A, 1
        oi      0
        bz      cmpsi2_eq       ; minued==subtrahend
cmpsi2_cmp
        lr      A, 0            ; sign(minued-subtrahend)
        xs      J               ; sign^overflow
        oi      1               ; W.Z=0, W.S=result
cmpsi2_eq:
        jmp     return          ; W.Z=1

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param 4:5 multiplicand
;;; @param 6:7 multiplier
;;; @return 4:5 result
;;; @clobber 4 5 6 7
umul16:
        pi      pushQ           ; save Q
        pi      push0           ; save 0
        clr
        lr      QU, A
        lr      QL, A           ; result=0
        br      umul16_check
umul16_loop:
        lr      A, 7
        ni      1
        bz      umul16_sr       ; lsb(multiplier)==0
        lr      A, QL
        as      5
        lr      QL, A
        lr      A, QU
        lnk
        as      4
        lr      QU, A           ; result += multiplicand
umul16_sr:
        clr
        lr      0, A            ; 0=carry
        lr      A, 6
        ni      1
        bz      umul16_sr_nz
        li      H'80'
        lr      0, A            ; set carry
umul16_sr_nz:
        lr      A, 6
        sr      1
        lr      6, A
        lr      A, 7
        sr      1
        xs      0               ; shift in carry
        lr      7, A            ; multiplier >>= 1
umul16_next:
        lr      A, 5
        as      5
        lr      5, A
        lr      A, 4
        lnk
        as      4
        lr      4, A            ; multiplicand <<= 1
umul16_check:
        lr      A, 6
        oi      0
        bnz     umul16_loop
        lr      A, 7
        oi      0
        bnz     umul16_loop     ; while multiplier != 0
        lr      A, QU
        lr      4, A
        lr      A, QL
        lr      5, A            ; 4:5=result
        pi      pop0            ; restore 0
        pi      popQ            ; restore Q
        jmp     return

;;; Signed multiplication: multiplicand *= multiplier
;;; @param @2: multiplicand
;;; @param @3: multiplier
;;; @clobber 4 5 6 7 A
mulsi2:
        lr      A, 2
        inc
        lr      IS, A           ; point LSB(@2)
        lr      A, D
        lr      5, A
        lr      A, S
        lr      4, A            ; 4:5=multiplicand
        oi      0
        bp      mulsi2_multiplier
        com
        lr      4, A            ; MSB(~multiplicand)
        lr      A, 5
        com
        inc
        lr      5, A            ; LSB(-multiplicand)
        lr      A, 4
        lnk
        lr      4, A            ; MSB(-multiplicand)
mulsi2_multiplier:
        lr      A, 3
        inc
        lr      IS, A           ; point LSB(@3)
        lr      A, D
        lr      7, A
        lr      A, S
        lr      6, A
        oi      0
        bp      mulsi2_multiply
        com
        lr      6, A            ; MSB(~multiplyer)
        lr      A, 7
        com
        inc
        lr      7, A            ; LSB(-multiplyer)
        lr      A, 6
        lnk
        lr      6, A            ; MSB(-multiplyer)
mulsi2_multiply:
        pi      call
        da      umul16
        lr      A, 3
        lr      IS, A
        lr      A, S
        lr      6, A            ; MSB(multiplyer)
        lr      A, 2
        lr      IS, A
        lr      A, S            ; MSB(multiplicand)
        xs      6               ; MSB(multiplicand^multiplyer)
        bp      mulsi2_store
        lr      A, 4
        com
        lr      4, A            ; MSB(~result)
        lr      A, 5
        com
        inc
        lr      5, A            ; LSB(-result)
        lr      A, 4
        lnk
        lr      4, A            ; MSB(-result)
mulsi2_store:
        lr      A, 4
        lr      I, A
        lr      A, 5
        lr      S, A            ; @2=result
        jmp     return

;;; Unsigned comparison: minuend - subtrahend
;;; @param 4:5 minuend
;;; @param 6:7 subtrahend
;;; @clobber J
;;; @return W.Z W.S
ucmp16:
        pi      push1
        pi      push0
        lr      A, 6
        com
        lr      0, A
        lr      A, 7
        com
        inc
        lr      1, A
        lr      A, 0
        lnk
        lr      0, A            ; 0:1=-subtrahend
        lr      A, 5
        as      1
        lr      1, A
        lr      A, 4
        lnk
        as      0
        lr      0, A            ; 0:1=minuend-subtrahend
        lr      J, W
        lr      A, J
        sl      4
        sl      1
        sl      1               ; sign=carry << 6
        lr      J, A
        lr      A, 0
        oi      0
        bnz     ucmp16_cmp
        lr      A, 1
        oi      0
        bz      ucmp16_eq
ucmp16_cmp:
        lr      A, J
        xi      H'80'
        oi      1               ; W.Z=0, W.S=~W.C
ucmp16_eq:     
        pi      pop0
        pi      pop1
        jmp     return

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram 4:5 dividend
;;; @praram 6:7 divisor
;;; @return 4:5 quotient
;;; @return 6:7 remainder
;;; @clobber 4 5 6 7 A
udiv16:
        pi      pushQ           ; save Q
        pi      push1           ; save 1
        pi      push0           ; save 0
        lis     1
        lr      0, A            ; bits=1
        lr      A, 6
        oi      0
        bnz     udiv16_prep
        lr      A, 7
        oi      0
        bnz     udiv16_prep
        pi      pop0
        pi      pop1
        pi      popQ
        jmp     return          ; divide by zero
udiv16_prep_loop:
        lr      A, 7
        as      7
        lr      7, A
        lr      A, 6
        lnk
        as      6
        lr      6, A            ; divisor <<= 1
        lr      A, 0
        inc
        lr      0, A            ; ++bits
udiv16_prep:
        lr      A, 6
        oi      0
        bp      udiv16_prep_loop ; while msb(divisor) == 0
        clr
        lr      QU, A
        lr      QL, A           ; Q=quotient
        br      udiv16_enter_loop
udiv16_loop:
        pi      pop0            ; restore bits
        ds      0               ; --bits
        bz      udiv16_return   ; while bits != 0
        clr
        lr      1, A            ; clear carry
        lr      A, 6
        ni      1
        bz      udiv16_sr_lsb
        li      H'80'
        lr      1, A            ; set carry
udiv16_sr_lsb:
        lr      A, 6
        sr      1
        lr      6, A
        lr      A, 7
        sr      1
        xs      1               ; shift in carry
        lr      7, A            ; divisor >>= 1
        lr      A, QL
        lr      1, A
        as      1
        lr      QL, A
        lr      A, QU
        lr      1, A
        lnk
        as      1
        lr      QU, A           ; quotient <<= 1
udiv16_enter_loop:
        pi      push0           ; save bits
        pi      call
        da      ucmp16          ; dividend <=> divisor
        bm      udiv16_loop     ; branch if dividend < divisor
        lr      A, 6
        com
        lr      0, A
        lr      A, 7
        com
        inc
        lr      1, A
        lr      A, 0
        lnk
        lr      0, A            ; 0:1=-divisor
        lr      A, 5
        as      1
        lr      5, A
        lr      A, 4
        lnk
        as      0
        lr      4, A            ; dividend-=divisor
        lr      A, QL
        inc
        lr      QL, A           ; quotient |= 1
        br      udiv16_loop
udiv16_return:
        lr      A, 4
        lr      6, A
        lr      A, 5
        lr      7, A            ; 6:7=remainder
        lr      A, QU
        lr      4, A
        lr      A, QL
        lr      5, A            ; 4:5=quotient
        pi      pop0            ; restore 0
        pi      pop1            ; restore 1
        pi      popQ            ; restore Q
        jmp     return

;;; Signed division: dividend /= divisor
;;; @param @2: dividend
;;; @param @3: divisor
;;; @clobber 4 5 6 7 A
divsi2:
        lr      A, 2
        inc
        lr      IS, A           ; point LSB(@2)
        lr      A, D
        lr      5, A
        lr      A, S
        lr      4, A            ; 4:5=dividend
        oi      0
        bp      divsi2_divisor
        com
        lr      4, A            ; MSB(~dividend)
        lr      A, 5
        com
        inc
        lr      5, A            ; LSB(-dividend)
        lr      A, 4
        lnk
        lr      4, A            ; MSB(-dividend)
divsi2_divisor:
        lr      A, 3
        inc
        lr      IS, A           ; point LSB(@3)
        lr      A, D
        lr      7, A
        lr      A, S
        lr      6, A
        oi      0
        bp      divsi2_divide
        com
        lr      6, A            ; MSB(~divisor)
        lr      A, 7
        com
        inc
        lr      7, A            ; LSB(-divisor)
        lr      A, 6
        lnk
        lr      6, A            ; MSB(-divisor)
divsi2_divide:
        pi      call
        da      udiv16
        lr      A, 3
        lr      IS, A
        lr      A, S
        lr      6, A            ; MSB(divisor)
        lr      A, 2
        lr      IS, A
        lr      A, S            ; MSB(dividend)
        xs      6               ; MSB(dividend^divisor)
        bp      divsi2_store
        lr      A, 4
        com
        lr      4, A            ; MSB(~result)
        lr      A, 5
        com
        inc
        lr      5, A            ; LSB(-result)
        lr      A, 4
        lnk
        lr      4, A            ; MSB(-result)
divsi2_store:
        lr      A, 4
        lr      I, A
        lr      A, 5
        lr      S, A            ; @2=result
        jmp     return
