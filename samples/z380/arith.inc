;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z380

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
        negw    HL              ; HL=-value
print_uint16:
        ld      A, H
        or      A, L
        ret     Z
        swap    HL
        ld      HL, 0           ; HL(31-16)=0, HL(15-0) kept
        swap    HL
        divuw   HL, 10          ; HL=value/10, HL(31-16)=value%10
        swap    HL
        push    HL              ; push remainder
        swap    HL
        call    print_uint16
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

;;; Signed division; dividend / divisor, rounded toward zero
;;; @param HL dividend
;;; @param DE divisor
;;; @return HL quotient
;;; @return DE remainder, with the dividend's sign
;;; @clobber A
div16:
        ld      A, H
        or      A, A
        push    AF              ; AF.S=sign(dividend)
        xor     A, D
        push    AF              ; AF.S=sign(quotient)
        bit     7, H
        jr      Z, div16_dividend
        negw    HL              ; HL=|dividend|
div16_dividend:
        swap    HL
        ld      HL, 0           ; HL(31-16)=0, HL(15-0) kept
        swap    HL
        bit     7, D
        jr      Z, div16_divide
        ex      DE, HL
        negw    HL
        ex      DE, HL          ; DE=|divisor|
div16_divide:
        divuw   HL, DE          ; HL=quotient, HL(31-16)=remainder
        swap    HL
        ld      D, H
        ld      E, L            ; DE=remainder
        swap    HL
        pop     AF
        jp      P, div16_remainder
        negw    HL              ; HL=-quotient
div16_remainder:
        pop     AF
        ret     P
        ex      DE, HL
        negw    HL              ; DE=-remainder
        ex      DE, HL
        ret
