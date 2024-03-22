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

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param RR14: multiplicand
;;; @param RR12: multiplier
;;; @return RR14: result
;;; @clobber R10-R15
umul16:
        ld      R10, R14        ; RR10=multiplicand
        ld      R11, R15
        ld      R14, #0         ; result=0
        ld      R15, #0
        jr      umul16_check
umul16_loop:
        rcf                     ; multiplier >>= 1
        rrc     R12
        rrc     R13
        jr      nc, umul16_next ; if lsb(multiplier) == 0
        add     R15, R11
        adc     R14, R10        ; result += multiplicand
umul16_next:
        rcf                     ; multiplicand <<= 1
        rlc     R11
        rlc     R10
umul16_check:
        incw    RR12
        decw    RR12
        jr      nz, umul16_loop ; while multiplier != 0
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
        or      R12, R12
        jr      pl, mulsi2_abs_muliplicand
        com     R12
        com     R13
        incw    RR12            ; multiplicand = -multiplicand
mulsi2_abs_muliplicand:
        or      R14, R14
        jr      pl, mulsi2_multiply
        com     R14
        com     R15
        incw    RR14            ; multiplier = -multiplier
mulsi2_multiply:
        call    umul16          ; result = multiplicand * multiplier
        ld      R12, @R4
        xor     R12, @R5        ; R12=sign(@R4)^sign(@R5)
        jr      pl, mulsi2_end
        com     R14
        com     R15
        incw    RR14            ; result = -result
mulsi2_end:
        ld      0(R4), R14
        ld      1(R4), R15
        ret

;;; Unsigned division: dividend / divisor = quotient ... reminder
;;; @praram RR14: dividend
;;; @praram RR12: divisor
;;; @return RR14: reminder
;;; @return RR12: quotient
;;; @clobber R10-R15
udiv16:
        push    R9
        incw    RR12
        decw    RR12
        jp      z, udiv16_end
        ld      R9, #1          ; R9=bits
        jp      udiv16_prep
udiv16_prep_loop:
        rcf                     ; divisor <<= 1
        rlc     R13
        rlc     R12
        inc     R9
udiv16_prep:                    ; while msb(divisor) == 0
        or      R12, R12
        jr      pl, udiv16_prep_loop
        ld      R10, #0
        ld      R11, #0         ; RR10=quotient
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
        djnz    R9, udiv16_loop ; RR14=reminder
        ld      R12, R10        ; RR12=quotient
        ld      R13, R11
udiv16_end:
        pop     R9
        ret

;;; Signed division: dividend *= divisor
;;; @param @R4: dividend
;;; @param @R5: divisor
;;; @clobber R10-R15
divsi2:
        ld      R14, 0(R4)
        ld      R15, 1(R4)
        ld      R12, 0(R5)
        ld      R13, 1(R5)
        or      R14, R14
        jr      pl, divsi2_abs_dividend
        com     R14
        com     R15
        incw    RR14            ; dividend = -dividend
divsi2_abs_dividend:
        or      R12, R12
        jr      pl, divsi2_divide
        com     R12
        com     R13
        incw    RR12            ; divisor = -divisor
divsi2_divide:
        call    udiv16
        ld      R14, @R4
        xor     R14, @R5        ; R14=sign(@R4)^sign(@R5)
        jr      pl, divsi2_end
        com     R12
        com     R13
        incw    RR12            ; dividend = = -dividend
divsi2_end:
        ld      0(R4), R12
        ld      1(R4), R13
        ret

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
