;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Echo what getchar receives; CR is echoed as CR LF. NUL halts.
echo_loop:
        jsr     @getchar
        mov.b   r0l, r0l
        beq     echo_halt
echo_char:
        jsr     @putchar
        cmp.b   #$0D, r0l
        bne     echo_loop
        mov.b   #$0A, r0l
        bra     echo_char
echo_halt:
        jmp     @halt_to_system

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
