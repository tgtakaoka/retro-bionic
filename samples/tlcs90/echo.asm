;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tlcs90
        include "tmp90c802.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     0FFF0H
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
        ld      sp, stack
        jp      init_usart

        org     ORG_SWI
        halt                    ; halt to system

        org     0100H
init_usart:
        ld      (USARTC), 0
        ld      (USARTC), 0
        ld      (USARTC), 0     ; safest way to sync mode
        ld      (USARTC), CMD_IR_bm
        nop
        nop
        ld      (USARTC), ASYNC_MODE
        nop
        nop
        ld      (USARTC), RX_EN_TX_EN

receive_loop:
        bit     ST_RxRDY_bp, (USARTS)
        jr      z, receive_loop
receive_data:
        ld      a, (USARTD)
        or      a, a
        jr      nz, transmit_loop
        swi
transmit_loop:
        bit     ST_TxRDY_bp, (USARTS)
        jr      z, transmit_loop
transmit_data:
        ld      (USARTD), a
        cp      a, 0DH
        jr      nz, receive_loop
        ld      a, 0AH
        jr      transmit_loop
