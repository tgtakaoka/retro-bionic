;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print int16_t value
;;;   xppc   P1
;;;   .dbyte &value
;;; @local P2[1:0] &value
;;; @local P2[7:0] buffer
;;; @local P2[9:8] return address
print_int16:
        ld      @1(P1)          ; point &value
        ld      @1(P1)
        st      @-10(P2)        ; P2[1:0]=&value
        ld      0(P1)           ; P1=return address
        st      1(P2)
        xpah    P1
        st      9(P2)           ; P2[9:8]=return address
        ld      0(P2)
        xpal    P1              ; P1=&value
        st      8(P2)
        ld      0(P1)
        st      vPRINT
        ld      1(P1)
        st      vPRINT+1        ; vPRINT=value
        jp      print_uint16    ; branch if value >= 0
        scl                     ; clear borrow
        ldi     0
        cad     vPRINT
        st      vPRINT
        ldi     0
        cad     vPRINT+1
        st      vPRINT+1        ; negate vPRINT
        ldi     L(ADDR(putchar))
        xpal    P1
        ldi     H(ADDR(putchar))
        xpah    P1
        ldi     '-'
        xppc    P1              ; print '-'
        jmp     print_uint16
;;; Print uint16_t value
;;; @param vPRINT value
;;; @local P2[7:0] buffer
;;; @local P2[9:8] return address
;;; @clobber E
vPRINT:         .dbyte  0
print_uint16:
        ldi     0
        st      0(P2)           ; buffer[0]=0
        ldi     1
        xae                     ; E=1
        ldi     L(ADDR(udiv16))
        xpal    P1
        ldi     H(ADDR(udiv16))
        xpah    P1
print_uint16_loop:
        ld      vPRINT
        st      (vDIVIDEND-ADDR(udiv16))(P1)
        ld      vPRINT+1
        st      (vDIVIDEND-ADDR(udiv16)+1)(P1)
        ldi     10
        st      (vDIVISOR-ADDR(udiv16))(P1)
        ldi     0
        st      (vDIVISOR-ADDR(udiv16)+1)(P1)
        xppc    P1              ; calculate vPRINT/=10
        ld      (vREMINDER-ADDR(udiv16))(P1) ; L(vREMINDER)
        ori     '0'
        st      E(P2)           ; buffer[E]=digit
        ccl
        ldi     1
        ade
        xae                     ; ++E
        ld      (vQUOTIENT-ADDR(udiv16))(P1)
        st      vPRINT
        ld      (vQUOTIENT-ADDR(udiv16)+1)(P1)
        st      vPRINT+1
        or      vPRINT
        jnz     print_uint16_loop ; while vPRINT!=0
        ldi     L(ADDR(putchar))
        xpal    P1
        ldi     H(ADDR(putchar))
        xpah    P1              ; P1=ADDR(putchar)
print_uint16_putdigit:
        lde
        scl                     ; clear borrow
        cai     1
        xae                     ; --E
        ld      E(P2)
        jz      print_int16_return
        xppc    P1              ; print digit
        jmp     print_uint16_putdigit
print_int16_return:
        ld      @8(P2)          ; discard locals
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return

;;; Add: summand += addend
;;;   xppc   P1
;;;   .dbyte &addend
;;;   .dbyte &summand
;;; @local P2[1:0] &addend, addend
;;; @local P2[3:2] &summand
;;; @local P2[5:4] return address
addsi2_exit:
        ld      @4(P2)          ; discard locals
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
addsi2:
        ld      @1(P1)          ; point to &addend
        ld      @1(P1)
        st      @-6(P2)         ; P2[1:0]=&addend
        ld      @1(P1)
        st      1(P2)
        ld      @1(P1)
        st      2(P2)           ; P2[3:2]=&summand
        ld      0(P1)           ; P1=return address
        st      3(P2)
        ld      0(P2)           ; P2[1:0]=&addend
        xpal    P1
        st      4(P2)           ; P2[5:4]=return address
        ld      1(P2)
        xpah    P1              ; P1=&addend
        st      5(P2)
        ld      0(P1)
        st      0(P2)           ; P2[1:0]=addend
        ld      1(P1)
        st      1(P2)
        ld      2(P2)           ; P2[3:2]=&summand
        xpal    P1
        ld      3(P2)
        xpah    P1              ; P1=&summand
        ccl                     ; clear carry
        ld      0(P1)
        add     0(P2)           ; L(summand+addend)
        st      0(P1)
        ld      1(P1)
        add     1(P2)           ; L(summand+addend)
        st      1(P1)
        jmp     addsi2_exit

;;; Subtract: minuend -= subtrahend
;;;   xppc   P1
;;;   .dbyte &subtrahend
;;;   .dbyte &minuend
;;; @local P2[1:0] &subtrahend, subtrahend
;;; @local P2[3:2] &minuend
;;; @local P2[5:4] return address
subsi2_exit:
        ld      @4(P2)          ; discard locals
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
subsi2:
        ld      @1(P1)          ; point to &subtrahend
        ld      @1(P1)
        st      @-6(P2)         ; P2[1:0]=&subtrahend
        ld      @1(P1)
        st      1(P2)
        ld      @1(P1)
        st      2(P2)           ; P2[3:2]=&minuend
        ld      0(P1)           ; P1=return address
        st      3(P2)
        ld      0(P2)           ; P2[1:0]=&subtrahend
        xpal    P1
        st      4(P2)           ; P2[5:4]=return address
        ld      1(P2)
        xpah    P1              ; P1=&subtrahend
        st      5(P2)
        ld      0(P1)
        st      0(P2)           ; P2[1:0]=subtrahend
        ld      1(P1)
        st      1(P2)
        ld      2(P2)           ; P2[3:2]=&minuend
        xpal    P1
        ld      3(P2)
        xpah    P1              ; P1=&minuend
        scl                     ; clear borrow
        ld      0(P1)
        cad     0(P2)           ; L(minuend-subtrahend)
        st      0(P1)
        ld      1(P1)
        cad     1(P2)           ; H(minuend-subtrahend)
        st      1(P1)
        jmp     subsi2_exit

;;; Signed compare: A = sign(minuend - subtrahend)
;;;   xppc   P1
;;;   .dbyte &subtrahend
;;;   .dbyte &minuend
;;; @return A=0  JZ
;;;         A=1  JP
;;;         A=-1
;;; @local P2[1:0] &subtrahend, subtrahend
;;; @local P2[3:2] &minuend, minuend - subtrahend
;;; @local P2[5:4] return address
;;; @local P2[6]   sign(minuend - subtrahend)
cmpsi2_exit:
        st      6(P2)           ; save sign(minuend-subtrahend)
        ld      @4(P2)          ; discard locals
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        ld      @1(P2)          ; sign(minuend-subtrahend)
        xppc    P1              ; return
cmpsi2:
        ld      @1(P1)          ; point to &subtrahend
        ld      @1(P1)
        st      @-7(P2)         ; P2[1:0]=&subtrahend
        ld      @1(P1)
        st      1(P2)
        ld      @1(P1)
        st      2(P2)           ; P2[3:2]=&minuend
        ld      0(P1)           ; P1=return address
        st      3(P2)
        ld      0(P2)           ; P2[1:0]=&subtrahend
        xpal    P1
        st      4(P2)           ; P2[5:4]=return address
        ld      1(P2)
        xpah    P1              ; P1=&subtrahend
        st      5(P2)
        ld      0(P1)
        st      0(P2)           ; P2[1:0]=subtrahend
        ld      1(P1)
        st      1(P2)
        ld      2(P2)           ; P2[3:2]=&minuend
        xpal    P1
        ld      3(P2)
        xpah    P1              ; P1=&minuend
        scl                     ; clear borrow
        ld      0(P1)
        cad     0(P2)           ; L(minuend-subtrahend)
        st      2(P2)           ; P2[3:2]=minuend-subtrahend
        ld      1(P1)
        cad     1(P2)
        st      3(P2)
        or      2(P2)
        jz      cmpsi2_exit     ; branch if result==0 with A=0
        csa
        ani     S_OV
        jnz     cmpsi2_ov       ; branch if V=1
        ld      3(P2)           ; H(minuend-subtrahend)
        jp      cmpsi2_gt       ; V=0, N=0
cmpsi2_lt:
        ldi     -1              ; V=0, N=1
        jmp     cmpsi2_exit
cmpsi2_ov:
        ld      1(P2)           ; H(result)
        jp      cmpsi2_lt       ; V=1, N=0
cmpsi2_gt:                      ; V=1, N=1
        ldi     1
        jmp     cmpsi2_exit

;;; Signed multiply; multiplicand *= multiplier
;;;   xppc   P1
;;;   .dbyte &multiplier
;;;   .dbyte &multiplicand
;;; @local P2[1:0] multiplier, &multiplier
;;; @local P2[3:2] multiplicand, &multiplicand
;;; @local P2[4]   H(multiplicand ^ multiplier)
;;; @local P2[6:5] return address
mulsi2_exit:
        ld      @5(P2)          ; discard locals
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
mulsi2:
        ld      @1(P1)          ; point to &multiplier
        ld      @1(P1)
        st      @-7(P2)         ; P2[1:0]=&multiplier
        ld      @1(P1)
        st      1(P2)
        ld      @1(P1)
        st      2(P2)           ; P2[3:2]=&multiplicand
        ld      0(P1)
        st      3(P2)
        ld      0(P2)           ; P2[1:0]=&multiplier
        xpal    P1
        st      5(P2)           ; P2[6:5]=return address
        ld      1(P2)
        xpah    P1              ; P1=&multiplier
        st      6(P2)
        ld      0(P1)
        st      0(P2)           ; P2[1:0]=multiplier
        ld      1(P1)
        st      1(P2)
        ld      2(P2)           ; P2[3:2]=&multiplicand
        xpal    P1
        ld      3(P2)
        xpah    P1              ; P1=&multiplicand
        ld      0(P1)
        st      2(P2)           ; P2[3:2]=multiplicand
        ld      1(P1)
        st      3(P2)
        xor     1(P2)           ; P2[1]=H(multiplier)
        st      4(P2)           ; P2[4]=H(multiplicand^multiplier)
        ld      1(P2)           ; H(multiplier)
        jp      mulsi2_multiplicand
        scl                     ; clear borrow
        ldi     0
        cad     0(P2)
        st      0(P2)
        ldi     0
        cad     1(P2)
        st      1(P2)           ; negate multiplier
mulsi2_multiplicand:
        ld      3(P2)           ; H(multiplicand)
        jp      umul16
        scl                     ; clear borrow
        ldi     0
        cad     2(P2)
        st      2(P2)
        ldi     0
        cad     3(P2)
        st      3(P2)           ; negate multiplicand
        jmp     umul16
mulsi2_exit_1:
        jmp     mulsi2_exit
;;; Unsigned multiply; product = multiplicand * multiplier
;;; @param P2[1:0] multiplier
;;; @param P2[3:2] multiplicand
;;; @return P1[1:0] product
;;; @clobber multiplier multiplicand
;;; @unchanged P1
umul16:
        ldi     0
        st      0(P1)
        st      1(P1)           ; P1[1:0]=product
        jmp     umul16_check
umul16_loop:
        ld      1(P2)           ; P2[1:0]=multiplier
        ccl
        rrl
        st      1(P2)
        ld      0(P2)
        rrl
        st      0(P2)           ; multiplier >>= 1
        csa
        ani     S_CY
        jz      umul16_next     ; if lsb(multiplier) == 0
        ccl
        ld      2(P2)           ; P2[3:2]=multiplicand
        add     0(P1)           ; P1[1:0]=product
        st      0(P1)
        ld      3(P2)
        add     1(P1)
        st      1(P1)           ; product += multiplicand
umul16_next:
        ccl
        ld      2(P2)           ; P2[3:2]=multiplicand
        add     2(P2)
        st      2(P2)
        ld      3(P2)
        add     3(P2)
        st      3(P2)           ; multiplicand <<= 1
umul16_check:
        ld      0(P2)           ; P2[1:0]=multiplier
        or      1(P2)
        jnz     umul16_loop     ; while multiplier != 0
umul16_end:
        ld      4(P2)           ; H(multiplier^multiplicand)
        jp      mulsi2_exit_1
        scl                     ; clear borrow
        ldi     0
        cad     0(P1)           ; P1[1:0]=product
        st      0(P1)
        ldi     0
        cad     1(P1)
        st      1(P1)           ; negate product
        jmp     mulsi2_exit_1

;;; Signed divide; dividend /= divisor ... remainder
;;;   xppc   P1
;;;   .dbyte &divisor
;;;   .dbyte &dividend
;;; @return vREMINDER |dividend| % |divisor|
;;; @local P2[1:0] &divisor
;;; @local P2[3:2] &dividend
;;; @local P2[5:4] return address
divsi2_exit:
        ld      @4(P2)          ; discard locals
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
divsi2:
        ld      @1(P1)          ; point to &divisor
        ld      @1(P1)
        st      @-6(P2)         ; P2[1:0]=&divisor
        ld      @1(P1)
        st      1(P2)
        ld      @1(P1)
        st      2(P2)           ; P2[3:2]=&dividend
        ld      0(P1)           ; P1=return address
        st      3(P2)
        ld      0(P2)           ; P2[1:0]=&divisor
        xpal    P1
        st      4(P2)           ; P2[5:4]=return address
        ld      1(P2)
        xpah    P1              ; P1=&divisor
        st      5(P2)
        ld      0(P1)
        st      vDIVISOR
        xae                     ; E=high(divisor)
        ld      1(P1)
        st      vDIVISOR+1
        jp      divsi2_dividend ; if divisor>=0
        scl                     ; clear borrow
        ldi     0
        cad     vDIVISOR
        st      vDIVISOR
        ldi     0
        cad     vDIVISOR+1
        st      vDIVISOR+1      ; negate divisor
divsi2_exit_trampoline
        jmp     divsi2_exit
divsi2_dividend:
        ld      2(P2)           ; P2[3:2]=&dividend
        xpal    P1
        ld      3(P2)
        xpah    P1              ; P1=&dividend
        ld      0(P1)
        st      vDIVIDEND
        ld      1(P1)
        st      vDIVIDEND+1
        xre
        xae                     ; E=high(dividend^divisor)
        ld      1(P1)
        jp      divsi2_divide   ; if dividend>=0
        scl                     ; clear borrow
        ldi     0
        cad     vDIVIDEND
        st      vDIVIDEND
        ldi     0
        cad     vDIVIDEND+1
        st      vDIVIDEND+1     ; negate dividend
divsi2_divide:
        ldi     L(ADDR(udiv16))
        xpal    P1
        ldi     H(ADDR(udiv16))
        xpah    P1
        xppc    P1              ; call udiv16
        lde
        jp      divsi2_store
        scl                     ; clear borrow
        ldi     0
        cad     vQUOTIENT
        st      vQUOTIENT
        ldi     0
        cad     vQUOTIENT+1
        st      vQUOTIENT+1     ; negate quotient
divsi2_store:
        ld      2(P2)
        xpal    P1
        ld      3(P2)
        xpah    P1              ; Pq=&dividend
        ld      vQUOTIENT
        st      0(P1)
        ld      vQUOTIENT+1
        st      1(P1)
        jmp     divsi2_exit_trampoline

;;; Unsigned divide; quotient = dividend / divisor ... remainder
;;; @param vDIVISOR divisor
;;; @param vDIVIDEND dividend
;;; @return vQUOTIENT quotient
;;; @return vDIVIDEND reminder
;;; @clobber E vDIVISOR
;;; @unchanged P1
vQUOTIENT:      .dbyte  0
vDIVIDEND:      .dbyte  0
vDIVISOR:       .dbyte  0
vREMINDER       = vDIVIDEND
udiv16_bits:    .byte   0
udiv16_borrow:  .byte   0
udiv16_adjust:
        ccl
        ld      vDIVIDEND
        add     vDIVISOR
        st      vDIVIDEND
        ld      vDIVIDEND+1
        add     vDIVISOR+1
        st      vDIVIDEND+1     ; dividend+=divisor
udiv16_exit:
        xppc    P1
udiv16:
        ld      vDIVISOR
        or      vDIVISOR+1
        jz      udiv16_exit     ; divide by zero
        ldi     1
        st      udiv16_bits
        st      udiv16_borrow   ; borrow=false
        jmp     udiv16_prep
udiv16_prep_loop:
        ccl
        ld      vDIVISOR
        add     vDIVISOR
        st      vDIVISOR
        ld      vDIVISOR+1
        add     vDIVISOR+1
        st      vDIVISOR+1      ; divisor<<=1
        ild     udiv16_bits     ; ++bits
udiv16_prep:
        ld      vDIVISOR+1
        jp      udiv16_prep_loop ; while msb(divisor)==0
        ldi     0
        st      vQUOTIENT
        st      vQUOTIENT+1     ; quotient=0
        jmp     udiv16_enter_loop
udiv16_loop:
        ccl
        ld      vDIVISOR+1
        rrl
        st      vDIVISOR+1
        ld      vDIVISOR
        rrl
        st      vDIVISOR        ; divisor>>=1
        ccl
        ld      vQUOTIENT
        add     vQUOTIENT
        st      vQUOTIENT
        ld      vQUOTIENT+1
        add     vQUOTIENT+1
        st      vQUOTIENT+1     ; quotient<<=1
udiv16_enter_loop:
        ld      udiv16_borrow
        jz      udiv16_add
udiv16_sub:
        scl                     ; clear borrow
        ld      vDIVIDEND
        cad     vDIVISOR
        st      vDIVIDEND
        ld      vDIVIDEND+1
        cad     vDIVISOR+1
        st      vDIVIDEND+1     ; dividend-=divisor
        jmp     udiv16_quotient
udiv16_add:
        ccl
        ld      vDIVIDEND
        add     vDIVISOR
        st      vDIVIDEND
        ld      vDIVIDEND+1
        add     vDIVISOR+1
        st      vDIVIDEND+1     ; dividend+=divisor
udiv16_quotient:
        csa
        ani     S_CY
        st      udiv16_borrow
        jz      udiv16_next    ; if dividend<divisor
        ild     vQUOTIENT      ; quotient++
udiv16_next:
        dld     udiv16_bits     ; --bits
        jnz     udiv16_loop     ; while bits!=0
        ld      udiv16_borrow
        jz      udiv16_adjust
        jmp     udiv16_exit
