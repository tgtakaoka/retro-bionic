;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Console through the USART's interrupts: received characters queue
;;; at rx_queue, characters to send at tx_queue. The USART interrupts
;;; with type VEC_RX or VEC_TX, which usart.inc defines.
rx_queue:       equ     2000H
rx_queue_size:  equ     128
tx_queue:       equ     2080H
tx_queue_size:  equ     128

;;; Initialize the queues and the USART
;;; @clobber AX BX CL
init_io:
        mov     bx, rx_queue
        mov     cl, rx_queue_size
        call    queue_init
        mov     bx, tx_queue
        mov     cl, tx_queue_size
        call    queue_init
        mov     ah, RX_EN_TX_DIS
        call    init_usart
        mov     al, VEC_RX
        out     USARTRV, al     ; RxRDY interrupt type
        mov     al, VEC_TX
        out     USARTTV, al     ; TxRDY interrupt type
        ret

;;; Get character
;;; @return AL
;;; @return F.C 0 if no character
getchar:
        push    bx
        mov     bx, rx_queue
        cli
        call    queue_remove
        sti
        pop     bx
        ret

;;; Put newline
;;; @clobber AL
newline:
        mov     al, 0DH
        call    putchar
        mov     al, 0AH
;;; Put character
;;; @param AL
putchar:
        push    ax
        push    bx
        mov     bx, tx_queue
putchar_retry:
        cli
        call    queue_add
        sti
        jnc     putchar_retry   ; branch if queue is full
        mov     al, RX_EN_TX_EN ; enable Tx
        out     USARTC, al
        pop     bx
        pop     ax
        ret

;;; Put space
;;; @clobber AL
putspace:
        mov     al, ' '
        jmp     putchar

;;; RxRDY interrupt: queue the received character
isr_rx:
        push    ax
        push    bx
        in      al, USARTS
        test    al, ST_RxRDY_bm
        jz      isr_rx_exit
        in      al, USARTD      ; receive character
        mov     bx, rx_queue
        call    queue_add
isr_rx_exit:
        pop     bx
        pop     ax
        iret

;;; TxRDY interrupt: send the next queued character, or disable Tx
;;; when the queue is empty
isr_tx:
        push    ax
        push    bx
        in      al, USARTS
        test    al, ST_TxRDY_bm
        jz      isr_tx_exit
        mov     bx, tx_queue
        call    queue_remove
        jnc     isr_tx_empty
        out     USARTD, al      ; send character
        jmp     isr_tx_exit
isr_tx_empty:
        mov     al, RX_EN_TX_DIS
        out     USARTC, al      ; disable Tx
isr_tx_exit:
        pop     bx
        pop     ax
        iret

        include "init_usart.inc"
        include "queue.inc"
