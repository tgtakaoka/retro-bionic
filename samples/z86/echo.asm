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
        jp      init_config

        org     %1000
stack:  equ     $

init_config:
        srp     #%F0
        setrp   %F0
        ;; Stack is on external memory
        ld      P01M, #P01M_P0ADDR LOR P01M_P1DATA
        ld      P2M, #%FF       ; Port 2 is input
        ld      SPH, #HIGH stack
        ld      SPL, #LOW stack
        srp     #%10
        setrp   %10

init_usart:
        ld      r12, #HIGH USARTC
        ld      r13, #LOW USARTC
        clr     r0
        lde     @rr12, r0
        lde     @rr12, r0
        lde     @rr12, r0       ; safest way to sync mode
        ld      r0, #CMD_IR_bm
        lde     @rr12, r0       ; reset
        nop
        nop
        ld      r0, #ASYNC_MODE
        lde     @rr12, r0       ; async 1stop 8data x16
        nop
        nop
        ld      r0, #RX_EN_TX_EN
        lde     @rr12, r0 ; RTS/DTR, error reset, Rx enable, Tx enable
        ld      r10, #HIGH USARTD
        ld      r11, #LOW USARTD

receive_loop:
        lde     r1, @rr12       ; USARTS
        tm      r1, #ST_RxRDY_bm
        jr      z, receive_loop
receive_data:
        lde     r0, @rr10       ; USARTD
        or      r0, r0
        jr      nz, transmit_loop
        halt
transmit_loop:
        lde     r1, @rr12       ; USARTS
        tm      r1, #ST_TxRDY_bm
        jr      z, transmit_loop
transmit_data:
        lde     @rr10, r0       ; USARTD
        cp      r0, #%0D
        jr      nz, receive_loop
        ld      r0, #%0A
        jr      transmit_loop
