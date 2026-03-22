;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z280.inc"
        include "usart.inc"

        org     1000H
stack:  equ     $

        org     ORG_RESET
        ld      C, CNTL_IOPAGE
        ld      HL, HIGH16 REFRESH_RATE
        ldctl   (C), HL
        xor     A
        out     (LOW REFRESH_RATE), A
        jr      ORG_RESET

        jp      init

        org     0100H
init:
        ld      SP, stack

init_usart:
        xor     A               ; clear A
        ld      BC, USARTC
        out     (C), A          ; (USARTC)
        out     (C), A          ; (USARTC)
        out     (C), A          ; safest way to sync mode
        ld      A, CMD_IR_bm
        out     (C), A          ; reset
        nop
        nop
        ld      A, ASYNC_MODE
        out     (C), A
        nop
        nop
        ld      A, RX_EN_TX_EN
        out     (C), A

loop:
        ld      BC, USARTS
receive_loop:
        in      A, (C)          ; (USARTS)
        bit     ST_RxRDY_bp, A
        jr      Z, receive_loop
receive_data:
        ld      BC, USARTD
        in      A, (C)          ; (USARTD)
        ld      D, A
        or      A, A
        jr      Z, halt_to_system
        ld      BC, USARTS
transmit_loop:
        in      A, (C)          ; (USARTS)
        bit     ST_TxRDY_bp, A
        jr      Z, transmit_loop
transmit_data:
        ld      A, D
        ld      BC, USARTD
        out     (C), A          ; (USARTD)
        cp      0DH
        jr      NZ, loop
        ld      B, 0AH
        jr      transmit_loop
halt_to_system:
;        ld      HL, ORG_RST38
        ld      (HL), 0FFH
        rst     38h
