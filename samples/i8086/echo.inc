;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Echo what the USART receives, polling it; CR is echoed as CR LF.
;;; NUL halts.
receive_loop:
        in      al, USARTS
        test    al, ST_RxRDY_bm
        jz      receive_loop
receive_data:
        in      al, USARTD
        or      al, al
        jz      halt_to_system
        mov     ah, al          ; save character
transmit_loop:
        in      al, USARTS
        test    al, ST_TxRDY_bm
        jz      transmit_loop
transmit_data:
        mov     al, ah
        out     USARTD, al
        cmp     al, 0DH
        jne     receive_loop
        mov     ah, 0AH
        jmp     transmit_loop
halt_to_system:
        hlt

