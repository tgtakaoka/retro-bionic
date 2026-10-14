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
        lde
        st      @-1(P2)         ; save E
        ld      vPRINT+1
        st      @-1(P2)
        ld      vPRINT
        st      @-1(P2)
        ldi     0
        st      @-1(P2)
        ldi     10
        st      @-1(P2)         ; P2[3:2]=vPRINT, P2[1:0]=10
        xppc    P1              ; calculate vPRINT/10
        ld      2(P2)
        st      vPRINT
        ld      3(P2)
        st      vPRINT+1        ; vPRINT/=10
        ld      4(P2)
        xae                     ; restore E
        ld      @5(P2)          ; L(remainder), discard
        ori     '0'
        st      E(P2)           ; buffer[E]=digit
        ccl
        ldi     1
        ade
        xae                     ; ++E
        ld      vPRINT
        or      vPRINT+1
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
;;; @local P2[1:0] product
;;; @local P2[2]   L(multiplicand)
;;; @clobber P2[1:0] E
MPROD   =       0
MLO     =       2
MPLIER  =       3               ; multiplier, offset past the locals
MCAND   =       5               ; multiplicand, offset past the locals
mul16_exit:
        xppc    P1              ; return, leaving P1=ADDR(mul16)
mul16:
        ld      0(P2)
        or      1(P2)
        jnz     mul16_nonzero   ; branch if multiplier != 0
        st      2(P2)
        st      3(P2)           ; multiplicand=0
        jmp     mul16_exit
mul16_nonzero:
        ldi     0
        st      @-MPLIER(P2)
        st      MPROD+1(P2)     ; product=0
;;; The low 16 bits of a product don't depend on the signs, so only a
;;; negative multiplier is negated, with the multiplicand to compensate.
        ld      MPLIER+1(P2)
        jp      mul16_hi
        scl                     ; clear borrow
        ldi     0
        cad     MPLIER(P2)
        st      MPLIER(P2)
        ldi     0
        cad     MPLIER+1(P2)
        st      MPLIER+1(P2)    ; negate multiplier
        scl                     ; clear borrow
        ldi     0
        cad     MCAND(P2)
        st      MCAND(P2)
        ldi     0
        cad     MCAND+1(P2)
        st      MCAND+1(P2)     ; negate multiplicand
;;; H(multiplier) only adds L(multiplicand) into H(product)
mul16_hi:
        ld      MPLIER+1(P2)
        jz      mul16_lo        ; branch if H(multiplier)==0
        xae                     ; E=H(multiplier)
        ld      MCAND(P2)
        st      MLO(P2)
mul16_hi_loop:
        ccl
        lde
        rrl
        xae                     ; H(multiplier) >>= 1
        csa
        jp      mul16_hi_next   ; branch if lsb(multiplier)==0
        ccl
        ld      MLO(P2)
        add     MPROD+1(P2)
        st      MPROD+1(P2)     ; H(product) += L(multiplicand)
mul16_hi_next:
        lde
        jz      mul16_lo        ; until H(multiplier)==0
        ccl
        ld      MLO(P2)
        add     MLO(P2)
        st      MLO(P2)         ; L(multiplicand) <<= 1
        jmp     mul16_hi_loop
mul16_return:
        jmp     mul16_exit
;;; L(multiplier) shifts in place, and E holds L(multiplicand)
mul16_lo:
        ld      MPLIER(P2)
        jz      mul16_store     ; branch if L(multiplier)==0
        ld      MCAND(P2)
        xae                     ; E=L(multiplicand)
mul16_lo_loop:
        ccl
        ld      MPLIER(P2)
        rrl
        st      MPLIER(P2)      ; L(multiplier) >>= 1
        jz      mul16_lo_last   ; branch if this is the last bit, which is 1
        csa
        jp      mul16_lo_next   ; branch if lsb(multiplier)==0
        ccl
        lde
        add     MPROD(P2)
        st      MPROD(P2)
        ld      MCAND+1(P2)
        add     MPROD+1(P2)
        st      MPROD+1(P2)     ; product += multiplicand
mul16_lo_next:
        ccl
        lde
        ade
        xae
        ld      MCAND+1(P2)
        add     MCAND+1(P2)
        st      MCAND+1(P2)     ; multiplicand <<= 1
        jmp     mul16_lo_loop
mul16_lo_last:
        ccl
        lde
        add     MPROD(P2)
        st      MCAND(P2)
        ld      MCAND+1(P2)
        add     MPROD+1(P2)
        st      MCAND+1(P2)     ; multiplicand=product+multiplicand
        ld      @MPLIER(P2)     ; discard locals
        jmp     mul16_return
mul16_store:
        ld      MPROD(P2)
        st      MCAND(P2)
        ld      MPROD+1(P2)
        st      MCAND+1(P2)     ; multiplicand=product
        ld      @MPLIER(P2)     ; discard locals
        jmp     mul16_return

;;; Signed divide; dividend /= divisor ... remainder
;;;   xppc   P1
;;;   .dbyte &divisor
;;;   .dbyte &dividend
;;; @local P2[1:0] &divisor, divisor, remainder
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
;;;   xppc   P1                 ; leaves P1=ADDR(udiv16)
;;; @return P2[1:0] |dividend| % |divisor|
;;; @clobber E
div16:
        ld      1(P2)           ; H(divisor)
        xor     3(P2)           ; H(dividend)
        st      @-DREM(P2)      ; P2[0]=sign
        ld      DREM+1(P2)
        jp      div16_dividend
        scl                     ; clear borrow
        ldi     0
        cad     DREM(P2)
        st      DREM(P2)
        ldi     0
        cad     DREM+1(P2)
        st      DREM+1(P2)      ; divisor=-divisor
div16_dividend:
        ld      DQUO+1(P2)
        jp      udiv16_divide
        scl                     ; clear borrow
        ldi     0
        cad     DQUO(P2)
        st      DQUO(P2)
        ldi     0
        cad     DQUO+1(P2)
        st      DQUO+1(P2)      ; dividend=-dividend
        jmp     udiv16_divide

;;; Unsigned divide on the stack; P2[3:2] /= P2[1:0] ... remainder
;;;   P1=ADDR(udiv16)
;;;   xppc   P1                 ; leaves P1=ADDR(udiv16)
;;; @return P2[1:0] dividend % divisor
;;; @local P2[0]   sign of quotient
;;; @local P2[1]   bits
;;; @local P2[3:2] dividend>>1, then H(remainder-divisor)
;;; @local P2[5:4] divisor
;;; @clobber E
DSIGN   =       0
DBITS   =       1
DHALF   =       2
DDVS    =       4
DREM    =       6               ; divisor, then remainder
DQUO    =       8               ; dividend, then quotient
udiv16_end:
        ld      DSIGN(P2)
        jp      udiv16_pop
        scl                     ; clear borrow
        ldi     0
        cad     DQUO(P2)
        st      DQUO(P2)
        ldi     0
        cad     DQUO+1(P2)
        st      DQUO+1(P2)      ; negate quotient
udiv16_pop:
        ld      @DREM(P2)       ; discard locals
udiv16_exit:
        xppc    P1              ; return, leaving P1=ADDR(udiv16)
udiv16:
        ldi     0
        st      @-DREM(P2)      ; P2[0]=sign
udiv16_divide:
        ld      DREM+1(P2)
        jnz     udiv16_to_word  ; branch if divisor>255
        ld      DREM(P2)
        jz      udiv16_zero     ; divide by zero
        jp      udiv16_byte     ; branch if divisor<128
        jmp     udiv16_to_word
udiv16_zero:
        ld      DQUO(P2)
        st      DREM(P2)
        ld      DQUO+1(P2)
        st      DREM+1(P2)      ; remainder=dividend
        ldi     0
        st      DQUO(P2)
        st      DQUO+1(P2)      ; quotient=0
        jmp     udiv16_pop

;;; A divisor below 128 keeps the remainder in E, and shifts the
;;; dividend out of L(quotient) as the quotient bits shift in.
udiv16_byte_bits:
        st      DBITS(P2)
        ccl                     ; the bit shifted in first ends above the quotient
udiv16_byte_loop:
        ld      DQUO(P2)
        add     DQUO(P2)
        st      DQUO(P2)        ; shift C into quotient, dividend bit into C
        lde
        ade
        xae                     ; E=remainder<<1|C
        lde
        scl                     ; clear borrow
        cad     DREM(P2)        ; C=1 if remainder>=divisor
        jp      udiv16_byte_sub ; branch if remainder>=divisor
        dld     DBITS(P2)       ; --bits
        jnz     udiv16_byte_loop
        jmp     udiv16_byte_end
udiv16_byte_sub:
        xae                     ; remainder-=divisor
        dld     DBITS(P2)       ; --bits
        jnz     udiv16_byte_loop ; while bits!=0
udiv16_byte_end:
        ld      DQUO(P2)
        add     DQUO(P2)
        st      DQUO(P2)        ; shift the last C into quotient
        lde
        st      DREM(P2)        ; remainder=E, H(remainder) is already 0
        jmp     udiv16_end
udiv16_to_end:
        jmp     udiv16_end
udiv16_to_word:
        jmp     udiv16_word
udiv16_byte:
        ld      DQUO+1(P2)
        jnz     udiv16_byte_hi  ; branch if dividend>255
        scl                     ; clear borrow
        ld      DQUO(P2)
        cad     DREM(P2)
        xae                     ; E=dividend-divisor
        csa
        jp      udiv16_zero     ; branch if dividend<divisor
        ld      DQUO(P2)
        sr
        scl                     ; clear borrow
        cad     DREM(P2)
        jp      udiv16_byte_lo4 ; branch if dividend>>1>=divisor
        lde
        st      DREM(P2)        ; remainder=dividend-divisor
        ldi     1
        st      DQUO(P2)        ; quotient=1
        jmp     udiv16_to_end
udiv16_byte_lo4:
        ld      DQUO(P2)
        sr
        sr
        sr
        sr
        xae                     ; E=dividend>>4
        lde
        scl                     ; clear borrow
        cad     DREM(P2)
        jp      udiv16_byte8    ; branch if dividend>>4>=divisor
udiv16_byte4:
        ld      DQUO(P2)
        rr
        rr
        rr
        rr
        ani     X'F0
        st      DQUO(P2)        ; L(quotient)=dividend<<4
        ldi     0
        st      DQUO+1(P2)      ; H(quotient)=0
        ldi     4               ; quotient has 4 bits
        jmp     udiv16_byte_bits
udiv16_byte8:
        scl                     ; clear borrow
        ld      DQUO+1(P2)
        cad     DREM(P2)
        csa
        jp      udiv16_byte_lo  ; branch if H(dividend)<divisor
        jmp     udiv16_to_word  ; quotient has more than 8 bits
udiv16_byte_lo:
        ld      DQUO+1(P2)
        xae                     ; E=H(dividend) as remainder
        ldi     0
        st      DQUO+1(P2)      ; H(quotient)=0
        ldi     8               ; quotient has 8 bits
        jmp     udiv16_byte_bits
udiv16_byte_hi:
        ani     X'F0
        jnz     udiv16_byte8    ; branch if dividend>4095
        ld      DQUO+1(P2)
        rr
        rr
        rr
        rr
        xae                     ; E=H(dividend)<<4
        ld      DQUO(P2)
        sr
        sr
        sr
        sr
        ore
        xae                     ; E=dividend>>4
        lde
        scl                     ; clear borrow
        cad     DREM(P2)
        csa
        jp      udiv16_byte4    ; branch if dividend>>4<divisor
        jmp     udiv16_byte8
udiv16_word_to_end:
        jmp     udiv16_to_end

udiv16_word:
        ld      DREM(P2)
        st      DDVS(P2)
        ld      DREM+1(P2)
        st      DDVS+1(P2)      ; divisor
        ld      DQUO(P2)
        st      DREM(P2)
        ld      DQUO+1(P2)
        st      DREM+1(P2)      ; remainder=dividend
        ldi     0
        st      DQUO(P2)
        st      DQUO+1(P2)      ; quotient=0
        scl                     ; clear borrow
        ld      DREM(P2)
        cad     DDVS(P2)
        ld      DREM+1(P2)
        cad     DDVS+1(P2)
        csa
        jp      udiv16_word_end ; branch if dividend<divisor
        ccl
        ld      DREM+1(P2)
        rrl
        st      DHALF+1(P2)
        ld      DREM(P2)
        rrl
        st      DHALF(P2)       ; half=dividend>>1
        ldi     1
        st      DBITS(P2)       ; bits=1
udiv16_prep:
        scl                     ; clear borrow
        ld      DHALF(P2)
        cad     DDVS(P2)
        ld      DHALF+1(P2)
        cad     DDVS+1(P2)
        csa
        jp      udiv16_prep_end ; until divisor>half
        ccl
        ld      DDVS(P2)
        add     DDVS(P2)
        st      DDVS(P2)
        ld      DDVS+1(P2)
        add     DDVS+1(P2)
        st      DDVS+1(P2)      ; divisor<<=1
        ild     DBITS(P2)       ; ++bits
        jmp     udiv16_prep
udiv16_prep_end:
        ld      DBITS(P2)
        scl                     ; clear borrow
        cai     8               ; bits-8
        jz      udiv16_lo       ; 8 bits into L(quotient)
        jp      udiv16_hi_bits  ; branch if bits>8
        jmp     udiv16_lo       ; bits into L(quotient)
udiv16_word_end:
        jmp     udiv16_word_to_end
udiv16_hi_bits:
        st      DBITS(P2)       ; bits-8 into H(quotient)
        jmp     udiv16_hi
udiv16_hi_loop:
        ccl
        ld      DDVS+1(P2)
        rrl
        st      DDVS+1(P2)
        ld      DDVS(P2)
        rrl
        st      DDVS(P2)        ; divisor>>=1
udiv16_hi:
        scl                     ; clear borrow
        ld      DREM(P2)
        cad     DDVS(P2)
        xae                     ; E=L(remainder-divisor)
        ld      DREM+1(P2)
        cad     DDVS+1(P2)
        st      DHALF(P2)       ; H(remainder-divisor)
        csa
        jp      udiv16_hi_next  ; branch if remainder<divisor
        ld      DHALF(P2)
        st      DREM+1(P2)
        lde
        st      DREM(P2)        ; remainder-=divisor
udiv16_hi_next:
        ld      DQUO+1(P2)
        add     DQUO+1(P2)
        st      DQUO+1(P2)      ; shift C into H(quotient)
        dld     DBITS(P2)       ; --bits
        jnz     udiv16_hi_loop  ; while bits!=0
        ldi     8
        st      DBITS(P2)       ; 8 bits into L(quotient)
udiv16_lo_loop:
        ccl
        ld      DDVS+1(P2)
        rrl
        st      DDVS+1(P2)
        ld      DDVS(P2)
        rrl
        st      DDVS(P2)        ; divisor>>=1
udiv16_lo:
        scl                     ; clear borrow
        ld      DREM(P2)
        cad     DDVS(P2)
        xae                     ; E=L(remainder-divisor)
        ld      DREM+1(P2)
        cad     DDVS+1(P2)
        st      DHALF(P2)       ; H(remainder-divisor)
        csa
        jp      udiv16_lo_next  ; branch if remainder<divisor
        ld      DHALF(P2)
        st      DREM+1(P2)
        lde
        st      DREM(P2)        ; remainder-=divisor
udiv16_lo_next:
        ld      DQUO(P2)
        add     DQUO(P2)
        st      DQUO(P2)        ; shift C into L(quotient)
        dld     DBITS(P2)       ; --bits
        jnz     udiv16_lo_loop  ; while bits!=0
        jmp     udiv16_word_end
