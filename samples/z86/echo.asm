;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z86c
        option  "reg-alias", "disable"

        include "z8.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     %FF00
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc LOR MODE_LEN8_gc LOR MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm LOR CMD_TxEN_bm

        org     ORG_RESET
        setrp   -1
        jp      init_config

        org     %1000
stack:  equ     $

init_config:
        ;; Stack is on external memory
        ld      P01M, #P01M_P0ADDR LOR P01M_P1DATA
        ld      P2M, #%FF       ; Port 2 is input
        ld      SPH, #HIGH stack
        ld      SPL, #LOW stack
        srp     #%10
        setrp   %10

init_usart:
        ld      R12, #HIGH USARTC
        ld      R13, #LOW USARTC
        clr     R0
        lde     @RR12, R0
        lde     @RR12, R0
        lde     @RR12, R0       ; safest way to sync mode
        ld      R0, #CMD_IR_bm
        lde     @RR12, R0       ; reset
        nop
        nop
        ld      R0, #ASYNC_MODE
        lde     @RR12, R0       ; async 1stop 8data x16
        nop
        nop
        ld      R0, #RX_EN_TX_EN
        lde     @RR12, R0 ; RTS/DTR, error reset, Rx enable, Tx enable
        ld      R10, #HIGH USARTD
        ld      R11, #LOW USARTD

receive_loop:
        lde     R1, @RR12       ; USARTS
        tm      R1, #ST_RxRDY_bm
        jr      z, receive_loop
receive_data:
        lde     R0, @RR10       ; USARTD
        or      R0, R0
        jr      nz, transmit_loop
        halt
transmit_loop:
        lde     R1, @RR12       ; USARTS
        tm      R1, #ST_TxRDY_bm
        jr      z, transmit_loop
transmit_data:
        lde     @RR10, R0       ; USARTD
        cp      R0, #%0D
        jr      nz, receive_loop
        ld      R0, #%0A
        jr      transmit_loop

        lde     @rr14, r15
        ld      r15, 0xaa
        ld      0xaa, #0xbb
