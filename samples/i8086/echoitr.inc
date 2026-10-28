;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Print each character received as "<char> 0xXX 0bXXXXXXXX", through
;;; the interrupt console of io.inc. NUL halts.
receive_loop:
        call    getchar
        jnc     receive_loop
        or      al, al
        jz      halt_to_system
echo_back:
        mov     dl, al          ; save character
        call    putchar         ; echo
        call    putspace
        call    put_hex8        ; print in hex
        call    putspace
        call    put_bin8        ; print in binary
        call    newline
        jmp     receive_loop
halt_to_system:
        hlt

;;; Print uint8_t in hex
;;; @param DL uint8_t value to be printed in hex.
;;; @clobber AL CL
put_hex8:
        mov     al, '0'
        call    putchar
        mov     al, 'x'
        call    putchar
        mov     al, dl
        mov     cl, 4
        shr     al, cl
        call    put_hex4
        mov     al, dl
put_hex4:
        and     al, 0FH
        cmp     al, 10
        jb      put_hex4_digit
        add     al, 'A'-'0'-10
put_hex4_digit:
        add     al, '0'
        jmp     putchar

;;; Print uint8_t in binary
;;; @param DL uint8_t value to be printed in binary.
;;; @clobber AL CX DH
put_bin8:
        mov     al, '0'
        call    putchar
        mov     al, 'b'
        call    putchar
        mov     dh, dl
        mov     cx, 8
put_bin8_loop:
        mov     al, '0'
        shl     dh, 1           ; F.C=MSB
        jnc     put_bin8_bit
        inc     al
put_bin8_bit:
        call    putchar
        loop    put_bin8_loop
        ret

