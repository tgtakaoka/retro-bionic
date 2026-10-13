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
        ld      (vREMAINDER-ADDR(udiv16))(P1) ; L(vREMAINDER)
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
;;; @local P2[1:0] &multiplier, multiplier
;;; @local P2[3:2] multiplicand, product
;;; @local P2[5:4] &multiplicand
;;; @local P2[7:6] return address
mulsi2_exit:
        ld      @6(P2)          ; discard locals
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
mulsi2:
        ld      @1(P1)          ; point to &multiplier
        ld      @1(P1)
        st      @-8(P2)         ; P2[1:0]=&multiplier
        ld      @1(P1)
        st      1(P2)
        ld      @1(P1)
        st      4(P2)           ; P2[5:4]=&multiplicand
        ld      0(P1)           ; P1=return address
        st      5(P2)
        ld      0(P2)
        xpal    P1
        st      6(P2)           ; P2[7:6]=return address
        ld      1(P2)
        xpah    P1              ; P1=&multiplier
        st      7(P2)
        ld      0(P1)
        st      0(P2)
        ld      1(P1)
        st      1(P2)           ; P2[1:0]=multiplier
        ld      4(P2)
        xpal    P1
        ld      5(P2)
        xpah    P1              ; P1=&multiplicand
        ld      0(P1)
        st      2(P2)
        ld      1(P1)
        st      3(P2)           ; P2[3:2]=multiplicand
        ldi     L(ADDR(mul16))
        xpal    P1
        ldi     H(ADDR(mul16))
        xpah    P1
        xppc    P1              ; P2[3:2]*=P2[1:0]
        ld      4(P2)
        xpal    P1
        ld      5(P2)
        xpah    P1              ; P1=&multiplicand
        ld      2(P2)
        st      0(P1)
        ld      3(P2)
        st      1(P1)           ; store product
        jmp     mulsi2_exit

;;; Signed multiply on the stack; P2[3:2] *= P2[1:0]
;;;   P1=ADDR(mul16)
;;;   xppc   P1                 ; leaves P1=ADDR(mul16)
;;; @local P2[0]   H(multiplicand ^ multiplier)
;;; @local P2[2:1] product
MSIGN   =       0
MPROD   =       1
MPLIER  =       3               ; multiplier, offset past the locals
MCAND   =       5               ; multiplicand, offset past the locals
mul16_exit:
        xppc    P1              ; return, leaving P1=ADDR(mul16)
mul16:
        ld      1(P2)           ; H(multiplier)
        xor     3(P2)           ; H(multiplicand)
        st      @-3(P2)         ; P2[0]=sign
        ld      MPLIER+1(P2)
        jp      mul16_multiplicand
        scl                     ; clear borrow
        ldi     0
        cad     MPLIER(P2)
        st      MPLIER(P2)
        ldi     0
        cad     MPLIER+1(P2)
        st      MPLIER+1(P2)    ; negate multiplier
mul16_multiplicand:
        ld      MCAND+1(P2)
        jp      mul16_zero
        scl                     ; clear borrow
        ldi     0
        cad     MCAND(P2)
        st      MCAND(P2)
        ldi     0
        cad     MCAND+1(P2)
        st      MCAND+1(P2)     ; negate multiplicand
mul16_zero:
        ldi     0
        st      MPROD(P2)
        st      MPROD+1(P2)     ; product=0
        jmp     mul16_check
mul16_return:
        jmp     mul16_exit
mul16_loop:
        ccl
        ld      MPLIER+1(P2)
        rrl
        st      MPLIER+1(P2)
        ld      MPLIER(P2)
        rrl
        st      MPLIER(P2)      ; multiplier >>= 1
        csa
        ani     S_CY
        jz      mul16_next      ; if lsb(multiplier) == 0
        ccl
        ld      MCAND(P2)
        add     MPROD(P2)
        st      MPROD(P2)
        ld      MCAND+1(P2)
        add     MPROD+1(P2)
        st      MPROD+1(P2)     ; product += multiplicand
mul16_next:
        ccl
        ld      MCAND(P2)
        add     MCAND(P2)
        st      MCAND(P2)
        ld      MCAND+1(P2)
        add     MCAND+1(P2)
        st      MCAND+1(P2)     ; multiplicand <<= 1
mul16_check:
        ld      MPLIER(P2)
        or      MPLIER+1(P2)
        jnz     mul16_loop      ; while multiplier != 0
        ld      MSIGN(P2)
        jp      mul16_store
        scl                     ; clear borrow
        ldi     0
        cad     MPROD(P2)
        st      MPROD(P2)
        ldi     0
        cad     MPROD+1(P2)
        st      MPROD+1(P2)     ; negate product
mul16_store:
        ld      MPROD(P2)
        st      MCAND(P2)
        ld      MPROD+1(P2)
        st      MCAND+1(P2)     ; multiplicand=product
        ld      @3(P2)          ; discard locals
        jmp     mul16_return

;;; Signed divide; dividend /= divisor ... remainder
;;;   xppc   P1
;;;   .dbyte &divisor
;;;   .dbyte &dividend
;;; @return vREMAINDER |dividend| % |divisor|
;;; @local P2[1:0] &divisor, divisor
;;; @local P2[3:2] dividend, quotient
;;; @local P2[5:4] &dividend
;;; @local P2[7:6] return address
divsi2_exit:
        ld      @6(P2)          ; discard locals
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
divsi2:
        ld      @1(P1)          ; point to &divisor
        ld      @1(P1)
        st      @-8(P2)         ; P2[1:0]=&divisor
        ld      @1(P1)
        st      1(P2)
        ld      @1(P1)
        st      4(P2)           ; P2[5:4]=&dividend
        ld      0(P1)           ; P1=return address
        st      5(P2)
        ld      0(P2)
        xpal    P1
        st      6(P2)           ; P2[7:6]=return address
        ld      1(P2)
        xpah    P1              ; P1=&divisor
        st      7(P2)
        ld      0(P1)
        st      0(P2)
        ld      1(P1)
        st      1(P2)           ; P2[1:0]=divisor
        ld      4(P2)
        xpal    P1
        ld      5(P2)
        xpah    P1              ; P1=&dividend
        ld      0(P1)
        st      2(P2)
        ld      1(P1)
        st      3(P2)           ; P2[3:2]=dividend
        ldi     L(ADDR(div16))
        xpal    P1
        ldi     H(ADDR(div16))
        xpah    P1
        xppc    P1              ; P2[3:2]/=P2[1:0]
        ld      4(P2)
        xpal    P1
        ld      5(P2)
        xpah    P1              ; P1=&dividend
        ld      2(P2)
        st      0(P1)
        ld      3(P2)
        st      1(P1)           ; store quotient
        jmp     divsi2_exit

;;; Signed divide on the stack; P2[3:2] /= P2[1:0] ... remainder
;;;   P1=ADDR(div16)
;;;   xppc   P1                 ; leaves P1=ADDR(div16)
;;; @return vREMAINDER |dividend| % |divisor|
;;; @clobber E
div16_exit:
        xppc    P1              ; return, leaving P1=ADDR(div16)
div16:
        ld      1(P2)           ; H(divisor)
        xor     3(P2)           ; H(dividend)
        xae                     ; E=sign
        ld      0(P2)
        st      vDIVISOR
        ld      1(P2)
        st      vDIVISOR+1
        jp      div16_dividend
        scl                     ; clear borrow
        ldi     0
        cad     vDIVISOR
        st      vDIVISOR
        ldi     0
        cad     vDIVISOR+1
        st      vDIVISOR+1      ; negate divisor
div16_dividend:
        ld      2(P2)
        st      vDIVIDEND
        ld      3(P2)
        st      vDIVIDEND+1
        jp      div16_divide
        scl                     ; clear borrow
        ldi     0
        cad     vDIVIDEND
        st      vDIVIDEND
        ldi     0
        cad     vDIVIDEND+1
        st      vDIVIDEND+1     ; negate dividend
div16_divide:
        ldi     H(ADDR(udiv16))
        xpah    P1
        st      @-1(P2)
        ldi     L(ADDR(udiv16))
        xpal    P1
        st      @-1(P2)         ; push return address
        xppc    P1              ; call udiv16
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1              ; pop return address
        lde
        jp      div16_store
        scl                     ; clear borrow
        ldi     0
        cad     vQUOTIENT
        st      vQUOTIENT
        ldi     0
        cad     vQUOTIENT+1
        st      vQUOTIENT+1     ; negate quotient
div16_store:
        ld      vQUOTIENT
        st      2(P2)
        ld      vQUOTIENT+1
        st      3(P2)
        jmp     div16_exit

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
vREMAINDER       = vDIVIDEND
udiv16_bits:    .byte   0
udiv16_borrow:  .byte   0
;;; offsets from vQUOTIENT, which P1 points to in udiv16
UQUO    =       0
UDVD    =       vDIVIDEND-vQUOTIENT
UDVS    =       vDIVISOR-vQUOTIENT
UBITS   =       udiv16_bits-vQUOTIENT
UBORROW =       udiv16_borrow-vQUOTIENT
udiv16_exit:
        xppc    P1              ; return, leaving P1=ADDR(udiv16)
udiv16:
        ld      vDIVISOR
        or      vDIVISOR+1
        jz      udiv16_exit     ; divide by zero
        ldi     H(vQUOTIENT)
        xpah    P1
        st      @-1(P2)
        ldi     L(vQUOTIENT)
        xpal    P1
        st      @-1(P2)         ; push return address, P1=vQUOTIENT
        ldi     1
        st      UBITS(P1)
        st      UBORROW(P1)     ; borrow=false
        jmp     udiv16_prep
udiv16_prep_loop:
        ccl
        ld      UDVS(P1)
        add     UDVS(P1)
        st      UDVS(P1)
        ld      UDVS+1(P1)
        add     UDVS+1(P1)
        st      UDVS+1(P1)      ; divisor<<=1
        ild     UBITS(P1)       ; ++bits
udiv16_prep:
        scl                     ; clear borrow
        ld      UDVD(P1)
        cad     UDVS(P1)
        ld      UDVD+1(P1)
        cad     UDVS+1(P1)
        csa
        ani     S_CY
        jz      udiv16_prep_end ; until divisor>dividend
        ld      UDVS+1(P1)
        jp      udiv16_prep_loop ; while msb(divisor)==0
udiv16_prep_end:
        ldi     0
        st      UQUO(P1)
        st      UQUO+1(P1)      ; quotient=0
        jmp     udiv16_enter_loop
udiv16_return:
        jmp     udiv16_exit
udiv16_loop:
        ccl
        ld      UDVS+1(P1)
        rrl
        st      UDVS+1(P1)
        ld      UDVS(P1)
        rrl
        st      UDVS(P1)        ; divisor>>=1
        ccl
        ld      UQUO(P1)
        add     UQUO(P1)
        st      UQUO(P1)
        ld      UQUO+1(P1)
        add     UQUO+1(P1)
        st      UQUO+1(P1)      ; quotient<<=1
udiv16_enter_loop:
        ld      UBORROW(P1)
        jz      udiv16_add
udiv16_sub:
        scl                     ; clear borrow
        ld      UDVD(P1)
        cad     UDVS(P1)
        st      UDVD(P1)
        ld      UDVD+1(P1)
        cad     UDVS+1(P1)
        st      UDVD+1(P1)      ; dividend-=divisor
        jmp     udiv16_quotient
udiv16_add:
        ccl
        ld      UDVD(P1)
        add     UDVS(P1)
        st      UDVD(P1)
        ld      UDVD+1(P1)
        add     UDVS+1(P1)
        st      UDVD+1(P1)      ; dividend+=divisor
udiv16_quotient:
        csa
        ani     S_CY
        st      UBORROW(P1)
        jz      udiv16_next     ; if dividend<divisor
        ild     UQUO(P1)        ; quotient++
udiv16_next:
        dld     UBITS(P1)       ; --bits
        jnz     udiv16_loop     ; while bits!=0
        ld      UBORROW(P1)
        jnz     udiv16_pop
        ccl
        ld      UDVD(P1)
        add     UDVS(P1)
        st      UDVD(P1)
        ld      UDVD+1(P1)
        add     UDVS+1(P1)
        st      UDVD+1(P1)      ; dividend+=divisor
udiv16_pop:
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1              ; pop return address
        jmp     udiv16_return
