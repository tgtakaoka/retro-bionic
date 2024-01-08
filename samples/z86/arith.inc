;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z86c
        option  optimize-index, on

;;; Print unsigned 16-bit integer as decimal
;;; @param rr0: value
;;; @clobber rr0 rr12 rr14
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
        ld      r14, r0
        ld      r15, r1
        ld      r12, #HIGH 10
        ld      r13, #LOW 10
        call    udiv16
        push    r15
        ld      r0, r12
        ld      r1, r13
        call    print_uint16_inner
        pop     r0
        add     r0, #'0'
        jp      putchar

;;; Print signed 16-bit integer as decimal
;;; @param rr0: value
;;; @clobber rr0
print_int16:
        or      r0, r0
        jr      pl, print_uint16
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
        ld      r14, 0(r5)
        ld      r15, 1(r5)
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
        ld      r14, 0(r4)
        ld      r15, 1(r4)
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
        ld      r14, 0(r4)
        ld      r15, 1(r4)
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
        ld      r14, 0(r4)
        ld      r15, 1(r4)
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

;;; Unsigned multiplication: result = multiplicand * multiplier
;;; @param rr14: multiplicand
;;; @param rr12: multiplier
;;; @return rr14: result
;;; @clobber r10-r15
umul16:
        ld      r10, r14        ; rr10=multiplicand
        ld      r11, r15
        ld      r14, #0         ; result=0
        ld      r15, #0
        jr      umul16_check
umul16_loop:
        rcf                     ; multiplier >>= 1
        rrc     r12
        rrc     r13
        jr      nc, umul16_next ; if lsb(multiplier) == 0
        add     r15, r11
        adc     r14, r10        ; result += multiplicand
umul16_next:
        rcf                     ; multiplicand <<= 1
        rlc     r11
        rlc     r10
umul16_check:
        incw    rr12
        decw    rr12
        jr      nz, umul16_loop ; while multiplier != 0
umul16_end:
        ret

;;; Signed multiplication: multiplicand *= multiplier
;;; @param @r4: multiplicand
;;; @param @r5: multiplier
;;; @clobber r10-r15
mulsi2:
        ld      r14, 0(r4)
        ld      r15, 1(r4)
        ld      r12, 0(r5)
        ld      r13, 1(r5)
        or      r12, r12
        jr      pl, mulsi2_abs_muliplicand
        com     r12
        com     r13
        incw    rr12            ; multiplicand = -multiplicand
mulsi2_abs_muliplicand:
        or      r14, r14
        jr      pl, mulsi2_multiply
        com     r14
        com     r15
        incw    rr14            ; multiplier = -multiplier
mulsi2_multiply:
        call    umul16          ; result = multiplicand * multiplier
        ld      r12, @r4
        xor     r12, @r5        ; r12=sign(@r4)^sign(@r5)
        jr      pl, mulsi2_end
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
        push    r9
        incw    rr12
        decw    rr12
        jp      z, udiv16_end
        ld      r9, #1          ; r9=bits
        jp      udiv16_prep
udiv16_prep_loop:
        rcf                     ; divisor <<= 1
        rlc     r13
        rlc     r12
        inc     r9
udiv16_prep:                    ; while msb(divisor) == 0
        or      r12, r12
        jr      pl, udiv16_prep_loop
        ld      r10, #0
        ld      r11, #0         ; rr10=quotient
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
        ld      r12, r10        ; rr12=quotient
        ld      r13, r11
udiv16_end:
        pop     r9
        ret

;;; Signed division: dividend *= divisor
;;; @param @r4: dividend
;;; @param @r5: divisor
;;; @clobber r10-r15
divsi2:
        ld      r14, 0(r4)
        ld      r15, 1(r4)
        ld      r12, 0(r5)
        ld      r13, 1(r5)
        or      r14, r14
        jr      pl, divsi2_abs_dividend
        com     r14
        com     r15
        incw    rr14            ; dividend = -dividend
divsi2_abs_dividend:
        or      r12, r12
        jr      pl, divsi2_divide
        com     r12
        com     r13
        incw    rr12            ; divisor = -divisor
divsi2_divide:
        call    udiv16
        ld      r14, @r4
        xor     r14, @r5        ; r14=sign(@r4)^sign(@r5)
        jr      pl, divsi2_end
        com     r12
        com     r13
        incw    rr12            ; dividend = = -dividend
divsi2_end:
        ld      0(r4), r12
        ld      1(r4), r13
        ret

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
