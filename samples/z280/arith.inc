;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z280

;;; Print signed 16-bit integer as decimal
;;; @param HL value
;;; @clobber A HL
print_int16:
        ld      A, H
        or      A, L
        jr      Z, print_uint16_zero
        bit     7, H
        jr      Z, print_uint16
        ld      A, '-'
        call    putchar
        neg     HL              ; HL=-value
print_uint16:
        push    DE
        call    print_uint16_loop
        pop     DE
        ret
print_uint16_loop:
        ld      A, H
        or      A, L
        ret     Z
        ld      DE, 0           ; DEHL=value, zero extended
        divuw   DEHL, 10        ; HL=value/10, DE=value%10
        push    DE              ; push remainder
        call    print_uint16_loop
        pop     HL              ; pop remainder
        ld      A, L
        add     A, '0'
        jp      putchar
print_uint16_zero:
        ld      A, '0'
        jp      putchar

;;; Signed comparison; minuend - subtrahend
;;; @param BC minuend
;;; @param DE subtrahend
;;; @return AF.Z, AF.S
;;; @clobber A
cmp16:
        push    HL
        ld      H, B
        ld      L, C
        subw    HL, DE          ; AF.V=overflow, AF.S=sign
        jr      Z, cmp16_return ; AF.Z=1
        ld      A, H            ; high(minuend-subtrahend)
        jp      PO, cmp16_sign  ; if no overflow
        xor     80H             ; AF.S=overflow^sign
cmp16_sign:
        or      1               ; clear AF.Z
cmp16_return:
        pop     HL
        ret
