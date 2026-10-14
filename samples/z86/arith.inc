;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z86c
        option  optimize-index, on

;;; Print unsigned 16-bit integer as decimal
;;; @param RR0: value
;;; @clobber RR0 RR12 RR14
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
        ld      R14, R0
        ld      R15, R1
        ld      R12, #HIGH 10
        ld      R13, #LOW 10
        call    udiv16
        push    R15
        ld      R0, R12
        ld      R1, R13
        call    print_uint16_inner
        pop     R0
        add     R0, #'0'
        jp      putchar

;;; Print signed 16-bit integer as decimal
;;; @param RR0: value
;;; @clobber RR0
print_int16:
        or      R0, R0
        jr      pl, print_uint16
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
        ld      R14, 0(R5)
        ld      R15, 1(R5)
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
        ld      R14, 0(R4)
        ld      R15, 1(R4)
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
        ld      R14, 0(R4)
        ld      R15, 1(R4)
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
        ld      R14, 0(R4)
        ld      R15, 1(R4)
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

;;; Signed multiplication: result = multiplicand * multiplier
;;; The low 16 bits of a product don't depend on the multiplicand's
;;; sign, so only a negative multiplier needs both operands negated.
;;; @param RR14: multiplicand
;;; @param RR12: multiplier
;;; @return RR14: result
;;; @clobber R10-R15
mul16:
        or      R12, R12
        jr      pl, umul16      ; if multiplier >= 0
        com     R12
        com     R13
        incw    RR12            ; multiplier = -multiplier
        com     R14
        com     R15
        incw    RR14            ; multiplicand = -multiplicand
        ;; fall through to umul16

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param RR14: multiplicand
;;; @param RR12: multiplier
;;; @return RR14: result
;;; @clobber R10-R15
umul16:
        ld      R10, R14        ; RR10=multiplicand
        ld      R11, R15
        clr     R14             ; hi(result)=0
        or      R12, R12
        jr      nz, umul16_high ; if hi(multiplier) != 0
umul16_low:
        clr     R15             ; lo(result)=0
        or      R13, R13
        jr      z, umul16_end   ; if lo(multiplier) == 0
        rcf                     ; lo(multiplier) >>= 1, clearing its msb
        rrc     R13
        jr      umul16_bit
umul16_high:
        rcf                     ; hi(multiplier) >>= 1
        rrc     R12
        jr      nc, umul16_high_next ; if lsb(multiplier) == 0
        add     R14, R15        ; hi(result) += lo(multiplicand)
umul16_high_next:
        add     R15, R15        ; lo(multiplicand) <<= 1
        or      R12, R12
        jr      nz, umul16_high ; while hi(multiplier) != 0
        jr      umul16_low
umul16_shift:
        add     R11, R11        ; multiplicand <<= 1
        adc     R10, R10
        sra     R13             ; lo(multiplier) >>= 1
umul16_bit:
        jr      nc, umul16_shift ; if lsb(multiplier) == 0
        jr      z, umul16_last  ; if no more bits in multiplier
        add     R15, R11
        adc     R14, R10        ; result += multiplicand
        jr      umul16_shift
umul16_last:
        add     R15, R11
        adc     R14, R10        ; result += multiplicand
umul16_end:
        ret

;;; Signed multiplication: multiplicand *= multiplier
;;; @param @R4: multiplicand
;;; @param @R5: multiplier
;;; @clobber R10-R15
mulsi2:
        ld      R14, 0(R4)
        ld      R15, 1(R4)
        ld      R12, 0(R5)
        ld      R13, 1(R5)
        call    mul16
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
        or      R12, R12
        jp      nz, udiv16_word ; if divisor >= 256
        or      R14, R14
        jr      nz, udiv16_wide ; if dividend >= 256
        cp      R15, R13
        jr      c, udiv16_none  ; if dividend < divisor
        ld      R10, R15
        swap    R10
        and     R10, #%0F       ; R10=dividend>>4
        cp      R10, R13
        jr      nc, udiv16_long ; if quotient >= 16 or divisor == 0
        ;; A quotient below 16 is counted by subtraction.
        ld      R10, #-1        ; R10=quotient
udiv16_count:
        inc     R10
        sub     R15, R13        ; dividend -= divisor
        jr      nc, udiv16_count ; while dividend >= 0
        add     R15, R13        ; RR14=remainder
        ld      R13, R10        ; RR12=quotient
        ret
udiv16_none:
        clr     R13             ; quotient=0, remainder=dividend
udiv16_zero:
        ret
udiv16_long:
        or      R13, R13
        jr      z, udiv16_zero  ; if divisor == 0
        jr      udiv16_low      ; remainder=0
udiv16_wide:
        or      R13, R13
        jr      z, udiv16_zero  ; if divisor == 0
        jp      mi, udiv16_word ; if divisor >= 128
        ;; A divisor below 128 keeps remainder<<1 within a byte, so
        ;; the quotient is found a byte at a time.
        cp      R14, R13
        jr      c, udiv16_low   ; if hi(quotient) == 0
udiv16_high:
        ld      R10, R14
        swap    R10
        and     R10, #%0F       ; R10=hi(dividend)>>4
        cp      R10, R13
        jr      nc, udiv16_high_bits ; if hi(quotient) >= 16
        ;; A hi(quotient) below 16 is counted by subtraction.
        ld      R12, #-1        ; R12=hi(quotient)
udiv16_high_count:
        inc     R12
        sub     R14, R13        ; hi(dividend) -= divisor
        jr      nc, udiv16_high_count ; while hi(dividend) >= 0
        add     R14, R13        ; R14=remainder
        jr      udiv16_low
udiv16_high_bits:
        clr     R10             ; R10=remainder
        ld      R11, #8         ; R11=bits
udiv16_high_loop:
        rlc     R14             ; ~quotient<<1 | C, next dividend bit to C
        rlc     R10             ; remainder = remainder<<1 | C
        cp      R10, R13        ; C=1 if remainder < divisor
        jr      c, udiv16_high_next
        sub     R10, R13        ; remainder -= divisor, C=0
udiv16_high_next:
        djnz    R11, udiv16_high_loop
        rlc     R14
        com     R14
        ld      R12, R14        ; hi(quotient)
        ld      R14, R10        ; remainder
udiv16_low:
        ld      R10, #8/2       ; R10=bits/2
udiv16_low_loop:
        rlc     R15             ; ~quotient<<1 | C, next dividend bit to C
        rlc     R14             ; remainder = remainder<<1 | C
        cp      R14, R13        ; C=1 if remainder < divisor
        jr      c, udiv16_low_next1
        sub     R14, R13        ; remainder -= divisor, C=0
udiv16_low_next1:
        rlc     R15
        rlc     R14
        cp      R14, R13
        jr      c, udiv16_low_next2
        sub     R14, R13
udiv16_low_next2:
        djnz    R10, udiv16_low_loop
        rlc     R15
        com     R15
        ld      R13, R15        ; lo(quotient)
        ld      R15, R14
        clr     R14             ; RR14=remainder
        ret
udiv16_word:
        ld      R10, R14
        ld      R11, R15
        sub     R11, R13        ; RR10=dividend-divisor
        sbc     R10, R12
        jr      nc, udiv16_word_divide ; if dividend >= divisor
        clr     R12             ; quotient=0, remainder=dividend
        clr     R13
        ret
udiv16_word_divide:
        push    R9
        ld      R9, #1          ; R9=bits
udiv16_prep_loop:
        sub     R11, R13        ; RR10=dividend-(divisor<<1)
        sbc     R10, R12
        jr      c, udiv16_prep_end ; until divisor<<1 > dividend
        add     R13, R13        ; divisor <<= 1
        adc     R12, R12
        inc     R9
        jr      udiv16_prep_loop
udiv16_prep_end:
        ld      R10, #%FF
        ld      R11, #%FF       ; RR10=~quotient
        jr      udiv16_enter_loop
udiv16_loop:
        rcf                     ; divisor >>= 1
        rrc     R12
        rrc     R13
udiv16_enter_loop:
        sub     R15, R13        ; dividend -= divisor
        sbc     R14, R12
        jr      nc, udiv16_next ; if dividend >= 0
        add     R15, R13        ; dividend += divisor
        adc     R14, R12        ; C=1, FLAG.D is always cleared
udiv16_next:
        rlc     R11             ; ~quotient = ~quotient<<1 | C
        rlc     R10
        djnz    R9, udiv16_loop ; RR14=remainder
        pop     R9
        com     R10
        com     R11
        ld      R12, R10        ; RR12=quotient
        ld      R13, R11
        ret

;;; Signed division: dividend / divisor = quotient ... remainder
;;; @param RR14: dividend
;;; @param RR12: divisor
;;; @return RR12: quotient, rounded toward zero
;;; @return RR14: remainder, with the dividend's sign
;;; @clobber R10-R15
div16:
        or      R12, R12
        jr      mi, div16_neg_divisor ; if divisor < 0
div16_pos_divisor:
        or      R14, R14
        jp      pl, udiv16      ; if dividend >= 0
        ld      R10, R14
        ld      R11, R15
        add     R11, R13
        adc     R10, R12
        decw    RR10            ; RR10=dividend+divisor-1
        jr      mi, div16_negative ; if -dividend >= divisor
        clr     R12             ; quotient=0, remainder=dividend
        clr     R13
        ret
div16_negative:
        com     R14
        com     R15
        incw    RR14            ; dividend = -dividend
        call    udiv16
        com     R14
        com     R15
        incw    RR14            ; remainder = -remainder
        com     R12
        com     R13
        incw    RR12            ; quotient = -quotient
        ret
div16_neg_divisor:
        com     R12
        com     R13
        incw    RR12            ; divisor = -divisor
        call    div16_pos_divisor
        com     R12
        com     R13
        incw    RR12            ; quotient = -quotient
        ret

;;; Signed division: dividend /= divisor
;;; @param @R4: dividend
;;; @param @R5: divisor
;;; @clobber R10-R15
divsi2:
        ld      R14, 0(R4)
        ld      R15, 1(R4)
        ld      R12, 0(R5)
        ld      R13, 1(R5)
        call    div16
        ld      0(R4), R12
        ld      1(R4), R13
        ret

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
