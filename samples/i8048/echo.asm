;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8048
        include "i8048.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     0FCH
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

        org     ORG_RESET
        jmp     init
        org     ORG_INT
        jmp     init
init:
        mov     R0, #USARTC
        clr     A
        movx    @R0, A
        movx    @R0, A
        movx    @R0, A          ; safest way to sync mode
        mov     A, #CMD_IR_bm   ; reset
        movx    @R0, A
        nop
        nop
        mov     A, #ASYNC_MODE
        movx    @R0, A
        nop
        nop
        mov     A, #RX_EN_TX_EN
        movx    @R0, A

receive_loop:
        mov     R0, #USARTS
        movx    A, @R0
        cpl     A
        jb      ST_RxRDY_bp, receive_loop
receive_data:
        mov     R0, #USARTD
        movx    A, @R0
        jz      halt_to_system
        mov     R2, A           ; save character
transmit_loop:
        mov     R0, #USARTS
        movx    A, @R0
        cpl     A
        jb      ST_TxRDY_bp, transmit_loop
transmit_data: 
        mov     R0, #USARTD
        mov     A, R2
        movx    @R0, A
        add     A, #-0DH
        jnz     receive_loop
        mov     R2, #0AH
        jmp     transmit_loop
halt_to_system:
        db      01H             ; halt to system
