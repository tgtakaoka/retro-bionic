;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z80
        include "z80.inc"

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
        jp      init

        org     0100H
init:
        ld      SP, stack

init_usart:
        xor     A               ; clear A
        out     (USARTC), A
        out     (USARTC), A
        out     (USARTC), A     ; safest way to sync mode
        ld      A, CMD_IR_bm
        out     (USARTC), A     ; reset
        nop
        nop
        ld      A, ASYNC_MODE
        out     (USARTC), A
        nop
        nop
        ld      A, RX_EN_TX_EN
        out     (USARTC), A

receive_loop:
        in      A, (USARTS)
        bit     ST_RxRDY_bp, A
        jr      Z, receive_loop
receive_data:
        in      A, (USARTD)
        ld      B, A
        or      A, A
        jr      Z, halt_to_system
transmit_loop:
        in      A, (USARTS)
        bit     ST_TxRDY_bp, A
        jr      Z, transmit_loop
transmit_data:
        ld      A, B
        out     (USARTD), A
        cp      0DH
        jr      NZ, receive_loop
        ld      B, 0AH
        jr      transmit_loop
halt_to_system:
        halt

        ex      af, af'
        exx

        rst     00h
        push    af
        push    bc
        push    de
        push    hl
        push    ix
        push    iy
        ld      a, i
        ld      (hl), a
        ld      a, r
        ld      (hl), a

        ld      a, 0
        ld      r, a
        ld      a, 0
        ld      i, a
        pop     af
        pop     bc
        pop     de
        pop     hl
        pop     ix
        pop     iy
        ld      sp, 1234h
        jp      5678h
