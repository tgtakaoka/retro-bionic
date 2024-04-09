;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8085
        include "i8085.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     00H
USARTD:         equ     USART+0 ; Receive/Transmit data
USARTS:         equ     USART+1 ; Srtatus register
USARTC:         equ     USART+1 ; Control register
USARTRV:        equ     USART+2 ; Receive interrupt vector (ORG_*)
USARTTV:        equ     USART+3 ; Transmit interrupt vector (ORG_*)
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jmp     init

        org     ORG_RST7
        jmp     isr_intr

        org     0100H
init:
        lxi     sp, stack
        lxi     h, rx_queue
        mvi     b, rx_queue_size
        call     queue_init
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
        mvi     a, RX_EN_TX_EN
        out     USARTC
        mvi     a, ORG_RST7
        out     USARTRV         ; enable RxRDY interrupt using RST 7
        mvi     a, ORG_RESET
        out     USARTTV         ; disable TxRDY interrupt

        ei

        lxi     h, rx_queue
receive_loop:
        di                      ; Disable INTR
        call    queue_remove
        ei                      ; Enable INTR
        jnc     receive_loop
        mov     b, a            ; save character
        ora     a
        jz      halt_to_system
transmit_loop:
        in      USARTC
        ani     ST_TxRDY_bm
        jz      transmit_loop
transmit_data:
        mov     a, b
        out     USARTD
        cpi     0DH
        jnz     receive_loop
        mvi     b, 0AH
        jmp     transmit_loop
halt_to_system:
        hlt

        include "queue.inc"

isr_intr:
        push    psw
        push    h
        in      USARTS
isr_intr_receive:
        ani     ST_RxRDY_bm
        jz      isr_intr_recv_end
        in      USARTD
        lxi     h, rx_queue
        call    queue_add
isr_intr_recv_end:
        pop     h
        pop     psw
        ei
        ret
