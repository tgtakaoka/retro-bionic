;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print signed 16-bit integer as decimal
;;; @param HL value
;;; @clobber A HL QHL
print_int16:
        cmp     HL, #0
        je      print_uint16_zero
        jgt     print_uint16
        ldb     A, #'-'
        scall   putchar
        neg     HL
print_uint16:
        cmp     HL, #0
        jne     print_uint16_div10
        ret
print_uint16_div10:
        ext     HL              ; extend to 32-bit
        divu    HL, #10
        push    QHL             ; push remainder
        scall   print_uint16
        pop     A               ; pop remainder
        addb    A, #'0'
        sjmp    putchar
print_uint16_zero:
        ldb     A, #'0'
        sjmp    putchar
