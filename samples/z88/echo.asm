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
        ldw     RR12, #USART
        clr     R0
        lde     USARTC(RR12), R0
        lde     USARTC(RR12), R0
        lde     USARTC(RR12), R0 ; safest way to sync mode
        ld      R0, #CMD_IR_bm
        lde     USARTC(RR12), R0 ; reset
        nop
        nop
        ld      R0, #ASYNC_MODE
        lde     USARTC(RR12), R0 ; async 1stop 8data x16
        nop
        nop
        ld      R0, #RX_EN_TX_EN
        lde     USARTC(RR12), R0 ; RTS/DTR, error reset, Rx enable, Tx enable

receive_loop:
        lde     R1, USARTS(RR12)
        tm      R1, #ST_RxRDY_bm
        jr      z, receive_loop
receive_data:
        lde     R0, USARTD(RR12)
        or      R0,R0
        jr      nz,transmit_loop
        wfi                     ; halt to system
transmit_loop:
        lde     R1, USARTS(RR12)
        tm      R1, #ST_TxRDY_bm
        jr      z, transmit_loop
transmit_data:
        lde     USARTD(RR12), R0
        cp      R0, #%0D
        jr      nz, receive_loop
        ld      R0, #%0A
        jr      transmit_loop

        srp1    #0
        lde     @rr14, r15
        ld      r15, 0xaa
        ld      0xaa, #0xbb
        ldw     SPH, #0x1234
