;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z88
        option  "reg-alias", "disable"
        option  "optimize-index", "enable"

        include "z88.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     %FF00
USARTD: equ     0       ; Receive/Transmit data
USARTS: equ     1       ; Status register
USARTC: equ     1       ; Control register
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc LOR MODE_LEN8_gc LOR MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm LOR CMD_TxEN_bm

        org     %1000
stack:  equ     $

        org     ORG_RESET
        setrp   %C0
init_config:
        ld      EMT, #EMT_STACK_DM ; stack is on external data memory
        ld      P0M, #P0M_ADRH     ; Port 0 is address bus high
        ld      PM, #PM_P1BUS | PM_DM ; Port 1 is data bus and address bus low
        ldw     SPH, #stack

init_usart:
        ldw     rr12, #USART
        clr     r0
        lde     USARTC(rr12), r0
        lde     USARTC(rr12), r0
        lde     USARTC(rr12), r0 ; safest way to sync mode
        ld      r0, #CMD_IR_bm
        lde     USARTC(rr12), r0 ; reset
        nop
        nop
        ld      r0, #ASYNC_MODE
        lde     USARTC(rr12), r0 ; async 1stop 8data x16
        nop
        nop
        ld      r0, #RX_EN_TX_EN
        lde     USARTC(rr12), r0 ; RTS/DTR, error reset, Rx enable, Tx enable

receive_loop:
        lde     r1, USARTS(rr12)
        tm      r1, #ST_RxRDY_bm
        jr      z, receive_loop
receive_data:
        lde     r0, USARTD(rr12)
        or      r0,r0
        jr      nz,transmit_loop
        wfi                     ; halt to system
transmit_loop:
        lde     r1, USARTS(rr12)
        tm      r1, #ST_TxRDY_bm
        jr      z, transmit_loop
transmit_data:
        lde     USARTD(rr12), r0
        cp      r0, #%0D
        jr      nz, receive_loop
        ld      r0, #%0A
        jr      transmit_loop
