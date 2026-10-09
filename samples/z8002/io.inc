;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Console through the USART's interrupts: received characters queue
;;; at rx_queue, characters to send at tx_queue. The USART interrupts on
;;; VI with identifier RX_ID or TX_ID, which usart.inc defines.
rx_queue:       equ     2000H
rx_queue_size:  equ     128
tx_queue:       equ     2080H
tx_queue_size:  equ     128

;;; Initialize the queues and the USART
;;; @clobber R0 R1
init_io:
        ld      r1, #rx_queue
        ldb     rl0, #rx_queue_size
        call    queue_init
        ld      r1, #tx_queue
        ldb     rl0, #tx_queue_size
        call    queue_init
        clrb    rl0
        outb    USARTC, rl0
        outb    USARTC, rl0
        outb    USARTC, rl0     ; safest way to sync mode
        ldb     rl0, #CMD_IR_bm
        outb    USARTC, rl0     ; reset
        nop
        nop
        ldb     rl0, #ASYNC_MODE
        outb    USARTC, rl0
        nop
        nop
        ldb     rl0, #RX_EN_TX_DIS
        outb    USARTC, rl0
        ldb     rl0, #RX_ID
        outb    USARTRV, rl0    ; RxRDY interrupt identifier
        ldb     rl0, #TX_ID
        outb    USARTTV, rl0    ; TxRDY interrupt identifier
        ret

;;; Get character
;;; @return RL0
;;; @return FLAGS.C 0 if no character
getchar:
        push    @r15, r1
        ld      r1, #rx_queue
        di      vi
        call    queue_remove
        ei      vi
        pop     r1, @r15
        ret

;;; Put newline
;;; @clobber RL0
newline:
        ldb     rl0, #0DH
        call    putchar
        ldb     rl0, #0AH
;;; Put character
;;; @param RL0
putchar:
        push    @r15, r1
        ld      r1, #tx_queue
putchar_retry:
        di      vi
        call    queue_add
        ei      vi
        jr      nc, putchar_retry ; branch if queue is full
        pop     r1, @r15
        push    @r15, r0
        ldb     rl0, #RX_EN_TX_EN ; enable Tx
        outb    USARTC, rl0
        pop     r0, @r15
        ret

;;; Put space
;;; @clobber RL0
putspace:
        ldb     rl0, #' '
        jr      putchar

isr_rx:
        push    @r15, r0
        push    @r15, r1
        inb     rl0, USARTS
        bitb    rl0, #ST_RxRDY_bp
        jr      z, isr_rx_exit
        inb     rl0, USARTD     ; receive character
        ld      r1, #rx_queue
        call    queue_add
isr_rx_exit:
        pop     r1, @r15
        pop     r0, @r15
        jp      isr_return

isr_tx:
        push    @r15, r0
        push    @r15, r1
        inb     rl0, USARTS
        bitb    rl0, #ST_TxRDY_bp
        jr      z, isr_tx_exit
        ld      r1, #tx_queue
        call    queue_remove
        jr      nc, isr_tx_empty
        outb    USARTD, rl0     ; send character
        jr      isr_tx_exit
isr_tx_empty:
        ldb     rl0, #RX_EN_TX_DIS
        outb    USARTC, rl0     ; disable Tx
isr_tx_exit:
        pop     r1, @r15
        pop     r0, @r15
        jp      isr_return

;;; System calls for a program in normal mode, which may not do I/O:
;;; SC #n with the arguments and results of the routine n names.
SC_GETCHAR:     equ     1       ; getchar
SC_PUTCHAR:     equ     2       ; putchar
SC_NEWLINE:     equ     3       ; newline

;;; The frame lies above R1 pushed here: the SC's identifier at 2, the
;;; caller's FCW at 4, which getchar's carry goes back in.
isr_sc:
        push    @r15, r1
        ld      r1, r15(#2)    ; SC #n: 7F00H|n
        cpb     rl1, #SC_GETCHAR
        jr      eq, isr_sc_getchar
        cpb     rl1, #SC_PUTCHAR
        jr      eq, isr_sc_putchar
        cpb     rl1, #SC_NEWLINE
        jr      ne, isr_sc_exit
        call    newline
        jr      isr_sc_exit
isr_sc_putchar:
        call    putchar
        jr      isr_sc_exit
isr_sc_getchar:
        call    getchar
        ld      r1, r15(#4)
        res     r1, #7          ; FCW C
        jr      nc, isr_sc_return
        set     r1, #7
isr_sc_return:
        ld      r15(#4), r1
isr_sc_exit:
        pop     r1, @r15
        jp      isr_return

        include "queue.inc"
