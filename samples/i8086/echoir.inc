;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Echo what the USART receives, its RxRDY interrupt queueing each
;;; character at rx_queue; CR is echoed as CR LF. NUL halts.
receive_loop:
        mov     bx, rx_queue
        cli
        call    queue_remove
        sti
        jnc     receive_loop
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

        include "init_usart.inc"
        include "queue.inc"

isr_intr:
        push    ax
        push    bx
        in      al, USARTS
        test    al, ST_RxRDY_bm
        jz      isr_intr_exit
        in      al, USARTD      ; receive character
        mov     bx, rx_queue
        call    queue_add
isr_intr_exit:
        pop     bx
        pop     ax
        iret

