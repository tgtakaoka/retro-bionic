;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Print each character received as "<char> 0xXX 0bXXXXXXXX". NUL
;;; halts.
echoitr_wait:
        sleep                   ; an interrupt wakes it
echoitr_loop:
        jsr     @getchar
        bcc     echoitr_wait
        mov.b   r0l, r0l
        beq     echoitr_halt
        mov.b   r0l, r3l
        jsr     @putchar        ; echo
        jsr     @putspace
        bsr     put_hex8        ; print in hex
        jsr     @putspace
        bsr     put_bin8        ; print in binary
        jsr     @newline
        bra     echoitr_loop
echoitr_halt:
        jmp     @halt_to_system

;;; Print uint8_t in hex
;;; @param R3L uint8_t value to be printed in hex.
;;; @clobber R0
put_hex8:
        mov.b   #"0", r0l
        jsr     @putchar
        mov.b   #"x", r0l
        jsr     @putchar
        mov.b   r3l, r0l
        shlr.b  r0l
        shlr.b  r0l
        shlr.b  r0l
        shlr.b  r0l
        bsr     put_hex4
        mov.b   r3l, r0l
put_hex4:
        and.b   #$0F, r0l
        cmp.b   #10, r0l
        blo     put_hex8_dec
        add.b   #"A"-10, r0l
        jmp     @putchar
put_hex8_dec:
        add.b   #"0", r0l
        jmp     @putchar

;;; Print uint8_t in binary
;;; @param R3L uint8_t value to be printed in binary.
;;; @clobber R0
put_bin8:
        push    r3
        mov.b   #"0", r0l
        jsr     @putchar
        mov.b   #"b", r0l
        jsr     @putchar
        bsr     put_bin4
        bsr     put_bin4
        pop     r3
        rts
put_bin4:
        bsr     put_bin2
put_bin2:
        bsr     put_bin1
put_bin1:
        mov.b   #"0", r0l
        shll.b  r3l             ; C=MSB
        bcc     put_bin1_out    ; MSB=0
        inc.b   r0l             ; MSB=1
put_bin1_out:
        jmp     @putchar

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
