;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     f3850

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     0F0H
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
        include "i8251.inc"

        org     0
        jmp     init_usart

        org     0100H
init_usart:
        lis     0
        out     USARTC
        out     USARTC
        out     USARTC          ; safest way to sync mode
;;; reset
        li      CMD_IR_bm
        out     USARTC
        nop
        nop
;;;  async 1stop 8data x16
        li      MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
        out     USARTC
        nop
        nop
;;;  RTS/DTR, error reset, Rx enable, Tx enable
        li      CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm
        out     USARTC

        lis     0
loop:   
        lr      is,a
        lr      i,a
        inc
        br7     $-2
        lr      i,a
        lr      a,is
        ai      8
        ci      0x40
        bnz     loop

receive_loop:
        in      USARTS
        ni      ST_RxRDY_bm
        bz      receive_loop
receive_data:
        in      USARTD
        lr      0, A
        bnz     transmit_loop
        dc      H'2F'
transmit_loop:
        in      USARTS
        ni      ST_TxRDY_bm
        bz      transmit_loop
transmit_data:
        lr      A, 0
        out     USARTD
        ci      H'0D'
        bnz     receive_loop
        lis     H'0A'
        lr      0, A
        br      transmit_loop
