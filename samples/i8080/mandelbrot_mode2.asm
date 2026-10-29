;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; The i8080 mandelbrot taking interrupts in mode 2, for chips such as
;;; the KL5C80A12 that have no mode 0: the interrupt setup and handler
;;; are the Z80 build's.
        include "i8080.inc"
        include "../z80/kl5c80a12.inc"
        include "../z80/usart.inc"

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jmp     init

        ;; Mode 2 vector for both the USART's Rx and Tx, and for the
        ;; KL5C80A12's IR0
        org     0080H
vector:
        dw      isr_intr

        org     0100H
init:
        lxi     sp, stack
        lxi     h, rx_queue
        mvi     b, rx_queue_size
        call    queue_init
        lxi     h, tx_queue
        mvi     b, tx_queue_size
        call    queue_init
init_usart:
        xra     a               ; clear A
        out     USARTC
        out     USARTC
        out     USARTC          ; safest way to sync mode
        mvi     a, CMD_IR_bm
        out     USARTC          ; reset
        nop
        nop
        mvi     a, ASYNC_MODE
        out     USARTC
        nop
        nop
        mvi     a, RX_EN_TX_DIS
        out     USARTC
        mvi     a, LOW vector
        out     USARTRV         ; set RxRDY interrupt vector
        out     USARTTV         ; set TxRDY interrupt vector
        ;; IR0 is left level triggered as reset leaves it.
        include "../z80/mode2.inc"
        cpu     8080
        ei

loop:
        call    mandelbrot
        call    newline
        jmp     loop

;;; Get character
;;; @return A
;;; @return F.C 0 if no character
getchar:
        push    h
        lxi     h, rx_queue
        di
        call    queue_remove
        ei
        pop     h
        ret

;;; Put character
;;; @param A
putchar:
        push    psw
        push    h
        lxi     h, tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jnc     putchar_retry   ; branch if queue is full
        pop     h
        mvi     a, RX_EN_TX_EN  ; enable Tx
        out     USARTC
putchar_exit:
        pop     psw
        ret

;;; Put newline
;;; @clobber A
newline:
        mvi     A, 0DH
        call    putchar
        mvi     A, 0AH
        jmp     putchar

;;; Put space
;;; @clobber A
putspace:
        mvi     A, ' '
        jmp     putchar

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

        include "../z80/usart_mode2.inc"

        end
