;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print signed 16-bit integer as decimal
;;; @param R1 value
;;; @clobber R0 R1
print_int16:
        movxwd  r1, r1          ; sign-extend
        cmpqd   0, r1
        bne     print_int16_nonzero
        movb    '0', r0
        br      putchar
print_int16_nonzero:
        cmpqd   0, r1
        bgt     print_int16_negative ; 0 > value
        br      print_uint16
print_int16_negative:
        movb    '-', r0
        bsr     putchar
        negd    r1, r1          ; R1=-value
print_uint16:
        cmpqd   0, r1
        bne     print_uint16_digit
        ret     0
print_uint16_digit:
        movd    r1, r0
        remd    10, r0          ; R0=value%10
        quod    10, r1          ; R1=value/10
        movd    r0, tos         ; push remainder
        bsr     print_uint16
        movd    tos, r0         ; pop remainder
        addd    '0', r0
        br      putchar
