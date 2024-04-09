;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8085
        include "i8085.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     00H
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jmp     init

        org     0100H
init:
        lxi     sp, stack

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

receive_loop:
        in      USARTS
        ani     ST_RxRDY_bm
        jz      receive_loop
receive_data:
        in      USARTD
        mov     b, a
        ora     a
        jz      halt_to_system
transmit_loop:
        in      USARTS
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
