;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print signed 16-bit integer as decimal
;;; @param R1 value
;;; @clobber R0 R1 R2 R3
print_int16:
        test    r1
        jr      nz, print_int16_nonzero
        ldb     rl0, #'0'
        jp      putchar
print_int16_nonzero:
        jr      pl, print_uint16
        ldb     rl0, #'-'
        call    putchar
        neg     r1              ; R1=-value
print_uint16:
        test    r1
        ret     z
        ld      r3, r1
        clr     r2              ; RR2=value, zero-extended
        div     rr2, #10        ; R3=value/10, R2=value%10
        push    @r15, r2        ; push remainder
        ld      r1, r3
        call    print_uint16
        pop     r0, @r15        ; pop remainder
        addb    rl0, #'0'
        jp      putchar
