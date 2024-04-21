;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8051
        include "i8051.inc"

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

        org     ORG_RESET
        ljmp    init
        org     ORG_IE0
        ljmp    init
        org     ORG_TF0
        ljmp    init
        org     ORG_IE1
        ljmp    init
        org     ORG_TF1
        ljmp    init
        org     ORG_RITI
        ljmp    init

init:
        mov     DPTR, #USARTC
        clr     A
        movx    @DPTR, A
        movx    @DPTR, A
        movx    @DPTR, A        ; safest way to sync mode
        mov     A, #CMD_IR_bm   ; reset
        movx    @DPTR, A
        nop
        nop
        mov     A, #ASYNC_MODE
        movx    @DPTR, A
        nop
        nop
        mov     A, #RX_EN_TX_EN
        movx    @DPTR, A

receive_loop:
        mov     DPTR, #USARTS
        movx    A, @DPTR
        jnb     ACC.ST_RxRDY_bp, receive_loop
eceive_data:
        dec     DPL             ; DPTR=USARTD
        movx    A, @DPTR
        jz      halt_to_system
        mov     R2, A           ; save character
transmit_loop:
        mov     DPTR, #USARTS
        movx    A, @DPTR
        jnb     ACC.ST_TxRDY_bp, transmit_loop
transmit_data:
        dec     DPL             ; DPTR=USARTD
        mov     A, R2
        movx    @DPTR, A
        cjne    A, #0DH, receive_loop
        mov     R2, #0AH
        sjmp    transmit_loop
halt_to_system:
        db      0A5H            ; halt to system
