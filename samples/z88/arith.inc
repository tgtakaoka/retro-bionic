;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z88
        option  optimize-index, on

;;; Print unsigned 16-bit integer as decimal
;;; @param rr0: value
;;; @clobber rr0
print_uint16:
        incw    rr0
        decw    rr0
        jr      nz, print_uint16_inner
        ld      r0, #'0'
        call    putchar
print_uint16_end:
        ret
print_uint16_inner:
        incw    rr0
        decw    rr0
        jr      z, print_uint16_end
        ldw     rr14, rr0
        ldw     rr12, #10
        call    udiv16
        push    r15
        ldw     rr0, rr12
        call    print_uint16_inner
        pop     r0
        add     r0, #'0'
        jp      putchar

;;; Print signed 16-bit integer as decimal
;;; @param rr0: value
;;; @clobber rr0
print_int16:
        btjrf   print_uint16, r0, #7
        push    r0
        ld      r0, #'-'
        call    putchar
        pop     r0
        com     r1
        com     r0
        incw    rr0
        jp      print_uint16

;;; Negation; result = -value
;;; @param @r4: result
;;; @param @r5: value
;;; @clobber r14, r15
negsi2:
        ldw     rr14, @r5
        com     r14
        com     r15
        incw    rr14
        ld      0(r4), r14
        ld      1(r4), r15
        ret

;;; Signed addition: summand += addend
;;; @param @r4: summand
;;; @param @r5: addend
;;; @clobber r14, r15
addsi2:
        ldw     rr14, @r4
        inc     r5
        add     r15, @r5
        dec     r5
        adc     r14, @r5
        ld      0(r4), r14
        ld      1(r4), r15
        ret

;;; Singed subtraction: minuend -= subtrahend
;;; @param @r4: minuend
;;; @param @r5: subtrahend
;;; @clobber r14, r15
subsi2:
        ldw     rr14, @r4
        inc     r5
        sub     r15, @r5
        dec     r5
        sbc     r14, @r5
        ld      0(r4), r14
        ld      1(r4), r15
        ret

;;; Signed comparison: minuend - subtrahend
;;; @param @r4: minuend
;;; @param @r5: subtrahend
;;; @clobber r14, r15
cmpsi2:
        ldw     rr14, @r4
        inc     r5
        sub     r15, @r5
        dec     r5
        sbc     r14, @r5        ; rr14=@r4-@r5
        call    addsub_flags    ; set C,Z,S,V
        ret

;;; Set add/sub flags
;;; @param @r4: operand 1
;;; @param @r5: operand 2
;;; @param rr14: result
;;; @param FLAGS
;;; @return FLAGS: C, Z, S, V
addsub_flags:
        push    r15
        push    r14
        push    r13
        ld      r13, FLAGS
        or      r13, #F_ZERO LOR F_OVERFLOW ; set Z, V
        or      r15, r14
        jr      z, addsub_flags_z
        xor     r13, #F_ZERO    ; clear Z
addsub_flags_z:
        xor     r14, @r4
        xor     r14, @r5        ; r14:7 carry into S
        jr      nc, addsub_flags_nc
        xor     r14, #%80       ; r14:7=C^(carry into S)
addsub_flags_nc:
        jr      mi, addsub_flags_v
        xor     r13, #F_OVERFLOW ; clear V
addsub_flags_v:
        ld      FLAGS, r13
        pop     r13
        pop     r14
        pop     r15
        ret

;;; Unsigned multiplication: product = multiplicand * multiplier
;;; @param rr14: multiplicand
;;; @param rr12: multiplier
;;; @return rr14: product
;;; @clobber rr10
umul16:
        ldw     rr10, rr14   ; rr10=multiplicand
        mult    rr14, r13    ; rr14=high(multiplicand)*low(multiplier)
        push    r15          ; save a part 1 of high(product)
        ld      r14, r11
        mult    rr14, r12    ; rr14=low(multiplicand)*high(multiplier)
        push    r15          ; save a part 2 of high(product)
        ld      r14, r11
        mult    rr14, r13     ; rr14=low(multiplicand)*low(multiplier)
        pop     r10
        add     r14, r10        ; add a part 1 of high(product)
        pop     r10
        add     r14, r10        ; add a part 2 of high(product)
        ret

;;; Signed multiplication: multiplicand *= multiplier
;;; @param @r4: multiplicand
;;; @param @r5: multiplier
;;; @clobber r10-r15
mulsi2:
        ldw     rr14, @r4
        ldw     rr12, @r5
        btjrf   mulsi2_abs_muliplicand, r12, #7
        com     r12
        com     r13
        incw    rr12            ; multiplicand = -multiplicand
mulsi2_abs_muliplicand:
        btjrf   mulsi2_multiply, r14, #7
        com     r14
        com     r15
        incw    rr14            ; multiplier = -multiplier
mulsi2_multiply:
        call    umul16          ; result = multiplicand * multiplier
        ld      r12, @r4
        xor     r12, @r5        ; r12=sign(@r4)^sign(@r5)
        btjrf   mulsi2_end, r12, #7
        com     r14
        com     r15
        incw    rr14            ; result = -result
mulsi2_end:
        ld      0(r4), r14
        ld      1(r4), r15
        ret

;;; Unsigned division: dividend / divisor = quotient ... reminder
;;; @praram rr14: dividend
;;; @praram rr12: divisor
;;; @return rr14: reminder
;;; @return rr12: quotient
;;; @clobber r10-r15
udiv16:
        incw    rr12
        decw    rr12
        jp      z, udiv16_end   ; divide by zero erro
        inc     r12
        dec     r12
        jr      nz, udiv16_16   ; branch if high(divisor)!=0
        ldw     rr10, rr14
        div     rr10, r13       ; calculate rr14/r13
        jr      ov, udiv16_8    ; branch if quotient is overflowed
        ld      r12, #0         ; r10=reminder, r11=quotient
        ld      r13, r11
        ld      r14, r12
        ld      r15, r10
        ret
udiv16_8:
        push    r9              ; save r9
        ld      r9, r13         ; r9=divisor
        push    r15             ; save low(dividend)
        ld      r11, r14        ; high(dividend)
        ld      r10, #0
        div     rr10, r9
        ld      r12, r11        ; high(quotient)=high(dividend)/divisor
        ld      r14, r10        ; high(reminder)=high(dividend)%divisor
        pop     r11             ; low(dividend)
        ld      r10, #0
        div     rr10, r9
        ld      r13, r11        ; low(quotient)=low(dividend)/divisor
        ld      r15, r10        ; low(reminder)=low(dividend)%divisor
        div     rr14, r9        ; r14=reminder, r15=quotient
        add     r13, r15        ; adjust quotient
        adc     r12, #0
        ld      r15, r14        ; reminder
        ld      r14, r12
        pop     r9
        ret
udiv16_16:
        push    r9
        ld      r9, #1          ; r9=bits
        jp      udiv16_prep
udiv16_prep_loop:
        rcf                     ; divisor <<= 1
        rlc     r13
        rlc     r12
        inc     r9
udiv16_prep:                    ; while msb(divisor) == 0
        btjrf   udiv16_prep_loop, r12, #7
        ldw     rr10, #0        ; rr10=quotient
        jp      udiv16_enter_loop
udiv16_loop:
        rcf                     ; divisor >>= 1
        rrc     r12
        rrc     r13
        rcf                     ; quotient <<= 1
        rlc     r11
        rlc     r10
udiv16_enter_loop:
        sub     r15, r13        ; dividend -= divisor
        sbc     r14, r12
        jp      c, udiv16_readd ; if dividend < 0
        inc     r11             ; quotient |= 1
        jp      udiv16_next
udiv16_readd:
        add     r15, r13        ; dividend += divisor
        adc     r14, r12        ; FLAG.D is always cleared
udiv16_next:
        djnz    r9, udiv16_loop ; rr14=reminder
        ldw     rr12, rr10      ; rr12=quotient
        pop     r9
udiv16_end:
        ret

;;; Signed division: dividend *= divisor
;;; @param @r4: dividend
;;; @param @r5: divisor
;;; @clobber r10-r15
divsi2:
        ldw     rr14, @r4
        ldw     rr12, @r5
        btjrf   divsi2_abs_dividend, r14, #7
        com     r14
        com     r15
        incw    rr14            ; dividend = -dividend
divsi2_abs_dividend:
        btjrf   divsi2_divide, r12, #7
        com     r12
        com     r13
        incw    rr12            ; divisor = -divisor
divsi2_divide:
        call    udiv16
        ld      r14, @r4
        xor     r14, @r5        ; r14=sign(@r4)^sign(@r5)
        btjrf   divsi2_end, r14, #7
        com     r12
        com     r13
        incw    rr12            ; dividend = = -dividend
divsi2_end:
        ld      0(r4), r12
        ld      1(r4), r13
        ret
