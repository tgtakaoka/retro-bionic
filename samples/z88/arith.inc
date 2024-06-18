;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z88
        option  optimize-index, on

;;; Print unsigned 16-bit integer as decimal
;;; @param RR0: value
;;; @clobber RR0
print_uint16:
        incw    RR0
        decw    RR0
        jr      nz, print_uint16_inner
        ld      R0, #'0'
        call    putchar
print_uint16_end:
        ret
print_uint16_inner:
        incw    RR0
        decw    RR0
        jr      z, print_uint16_end
        ldw     RR14, RR0
        ldw     RR12, #10
        call    udiv16
        push    R15
        ldw     RR0, RR12
        call    print_uint16_inner
        pop     R0
        add     R0, #'0'
        jp      putchar

;;; Print signed 16-bit integer as decimal
;;; @param RR0: value
;;; @clobber RR0
print_int16:
        btjrf   print_uint16, R0, #7
        push    R0
        ld      R0, #'-'
        call    putchar
        pop     R0
        com     R1
        com     R0
        incw    RR0
        jp      print_uint16

;;; Negation; result = -value
;;; @param @R4: result
;;; @param @R5: value
;;; @clobber R14, R15
negsi2:
        ldw     RR14, @R5
        com     R14
        com     R15
        incw    RR14
        ld      0(R4), R14
        ld      1(R4), R15
        ret

;;; Signed addition: summand += addend
;;; @param @R4: summand
;;; @param @R5: addend
;;; @clobber R14, R15
addsi2:
        ldw     RR14, @R4
        inc     R5
        add     R15, @R5
        dec     R5
        adc     R14, @R5
        ld      0(R4), R14
        ld      1(R4), R15
        ret

;;; Singed subtraction: minuend -= subtrahend
;;; @param @R4: minuend
;;; @param @R5: subtrahend
;;; @clobber R14, R15
subsi2:
        ldw     RR14, @R4
        inc     R5
        sub     R15, @R5
        dec     R5
        sbc     R14, @R5
        ld      0(R4), R14
        ld      1(R4), R15
        ret

;;; Signed comparison: minuend - subtrahend
;;; @param @R4: minuend
;;; @param @R5: subtrahend
;;; @clobber R14, R15
cmpsi2:
        ldw     RR14, @R4
        inc     R5
        sub     R15, @R5
        dec     R5
        sbc     R14, @R5        ; RR14=@R4-@R5
        call    addsub_flags    ; set C,Z,S,V
        ret

;;; Set add/sub flags
;;; @param @R4: operand 1
;;; @param @R5: operand 2
;;; @param RR14: result
;;; @param FLAGS
;;; @return FLAGS: C, Z, S, V
addsub_flags:
        push    R15
        push    R14
        push    R13
        ld      R13, FLAGS
        or      R13, #F_ZERO LOR F_OVERFLOW ; set Z, V
        or      R15, R14
        jr      z, addsub_flags_z
        xor     R13, #F_ZERO    ; clear Z
addsub_flags_z:
        xor     R14, @R4
        xor     R14, @R5        ; R14:7 carry into S
        jr      nc, addsub_flags_nc
        xor     R14, #%80       ; R14:7=C^(carry into S)
addsub_flags_nc:
        jr      mi, addsub_flags_v
        xor     R13, #F_OVERFLOW ; clear V
addsub_flags_v:
        ld      FLAGS, R13
        pop     R13
        pop     R14
        pop     R15
        ret

;;; Unsigned multiplication: product = multiplicand * multiplier
;;; @param RR14: multiplicand
;;; @param RR12: multiplier
;;; @return RR14: product
;;; @clobber RR10
umul16:
        ldw     RR10, RR14   ; RR10=multiplicand
        mult    RR14, R13    ; RR14=high(multiplicand)*low(multiplier)
        push    R15          ; save a part 1 of high(product)
        ld      R14, R11
        mult    RR14, R12    ; RR14=low(multiplicand)*high(multiplier)
        push    R15          ; save a part 2 of high(product)
        ld      R14, R11
        mult    RR14, R13     ; RR14=low(multiplicand)*low(multiplier)
        pop     R10
        add     R14, R10        ; add a part 1 of high(product)
        pop     R10
        add     R14, R10        ; add a part 2 of high(product)
        ret

;;; Signed multiplication: multiplicand *= multiplier
;;; @param @R4: multiplicand
;;; @param @R5: multiplier
;;; @clobber R10-R15
mulsi2:
        ldw     RR14, @R4
        ldw     RR12, @R5
        btjrf   mulsi2_abs_muliplicand, R12, #7
        com     R12
        com     R13
        incw    RR12            ; multiplicand = -multiplicand
mulsi2_abs_muliplicand:
        btjrf   mulsi2_multiply, R14, #7
        com     R14
        com     R15
        incw    RR14            ; multiplier = -multiplier
mulsi2_multiply:
        call    umul16          ; result = multiplicand * multiplier
        ld      R12, @R4
        xor     R12, @R5        ; R12=sign(@R4)^sign(@R5)
        btjrf   mulsi2_end, R12, #7
        com     R14
        com     R15
        incw    RR14            ; result = -result
mulsi2_end:
        ld      0(R4), R14
        ld      1(R4), R15
        ret

;;; Unsigned division: dividend / divisor = quotient ... remainder
;;; @praram RR14: dividend
;;; @praram RR12: divisor
;;; @return RR14: remainder
;;; @return RR12: quotient
;;; @clobber R10-R15
udiv16:
        incw    RR12
        decw    RR12
        jp      z, udiv16_end   ; divide by zero erro
        inc     R12
        dec     R12
        jr      nz, udiv16_16   ; branch if high(divisor)!=0
        ldw     RR10, RR14
        div     RR10, R13       ; calculate RR14/R13
        jr      ov, udiv16_8    ; branch if quotient is overflowed
        ld      R12, #0         ; R10=remainder, R11=quotient
        ld      R13, R11
        ld      R14, R12
        ld      R15, R10
        ret
udiv16_8:
        push    R9              ; save R9
        ld      R9, R13         ; R9=divisor
        push    R15             ; save low(dividend)
        ld      R11, R14        ; high(dividend)
        ld      R10, #0
        div     RR10, R9
        ld      R12, R11        ; high(quotient)=high(dividend)/divisor
        ld      R14, R10        ; high(remainder)=high(dividend)%divisor
        pop     R11             ; low(dividend)
        ld      R10, #0
        div     RR10, R9
        ld      R13, R11        ; low(quotient)=low(dividend)/divisor
        ld      R15, R10        ; low(remainder)=low(dividend)%divisor
        div     RR14, R9        ; R14=remainder, R15=quotient
        add     R13, R15        ; adjust quotient
        adc     R12, #0
        ld      R15, R14        ; remainder
        ld      R14, R12
        pop     R9
        ret
udiv16_16:
        push    R9
        ld      R9, #1          ; R9=bits
        jp      udiv16_prep
udiv16_prep_loop:
        rcf                     ; divisor <<= 1
        rlc     R13
        rlc     R12
        inc     R9
udiv16_prep:                    ; while msb(divisor) == 0
        btjrf   udiv16_prep_loop, R12, #7
        ldw     RR10, #0        ; RR10=quotient
        jp      udiv16_enter_loop
udiv16_loop:
        rcf                     ; divisor >>= 1
        rrc     R12
        rrc     R13
        rcf                     ; quotient <<= 1
        rlc     R11
        rlc     R10
udiv16_enter_loop:
        sub     R15, R13        ; dividend -= divisor
        sbc     R14, R12
        jp      c, udiv16_readd ; if dividend < 0
        inc     R11             ; quotient |= 1
        jp      udiv16_next
udiv16_readd:
        add     R15, R13        ; dividend += divisor
        adc     R14, R12        ; FLAG.D is always cleared
udiv16_next:
        djnz    R9, udiv16_loop ; RR14=remainder
        ldw     RR12, RR10      ; RR12=quotient
        pop     R9
udiv16_end:
        ret

;;; Signed division: dividend *= divisor
;;; @param @R4: dividend
;;; @param @R5: divisor
;;; @clobber R10-R15
divsi2:
        ldw     RR14, @R4
        ldw     RR12, @R5
        btjrf   divsi2_abs_dividend, R14, #7
        com     R14
        com     R15
        incw    RR14            ; dividend = -dividend
divsi2_abs_dividend:
        btjrf   divsi2_divide, R12, #7
        com     R12
        com     R13
        incw    RR12            ; divisor = -divisor
divsi2_divide:
        call    udiv16
        ld      R14, @R4
        xor     R14, @R5        ; R14=sign(@R4)^sign(@R5)
        btjrf   divsi2_end, R14, #7
        com     R12
        com     R13
        incw    RR12            ; dividend = = -dividend
divsi2_end:
        ld      0(R4), R12
        ld      1(R4), R13
        ret
