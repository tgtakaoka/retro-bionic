;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "i8096.inc"
        include "usart.inc"

        org     1000H
stack:  equ     $

        org     1AH
A:      dsb     1
B:      dsb     1

        org     ORG_RESET
init:
        ld      SP, #stack
        ldb     INT_MASK, INT_EXTINT ; enable EXTINT
        ei                           ; for halt switch

init_usart:
        stb     ZERO, USARTC
        stb     ZERO, USARTC
        stb     ZERO, USARTC    ; safest way to sync mode
        ldb     A, #CMD_IR_bm
        stb     A, USARTC       ; reset
        nop
        nop
        ldb     A, #ASYNC_MODE
        stb     A, USARTC
        nop
        nop
        ldb     A, #RX_EN_TX_EN
        stb     A, USARTC

receive_loop:
        ldb     A, USARTS
        jbc     A, ST_RxRDY_bp, receive_loop
receive_data:
        ldb     B, USARTD
        orb     B, B
        je      halt_to_system
transmit_loop:
        ldb     A, USARTS
        jbc     A, ST_TxRDY_bp, transmit_loop
transmit_data:
        stb     B, USARTD
        cmpb    B, #0DH
        jne     receive_loop
        ldb     B, #0AH
        sjmp    transmit_loop

halt_to_system:
        ld      A, #VEC_TRAP
        st      A, VEC_TRAP     ; halt to system
        trap
