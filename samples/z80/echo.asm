;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z80.inc"
        include "usart.inc"

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
