
;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8048
        include "i8048.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     0FCH
USARTD:         equ     USART+0 ; Receive/Transmit data
USARTS:         equ     USART+1 ; Srtatus register
USARTC:         equ     USART+1 ; Control register
USARTRV:        equ     USART+2 ; Receive interrupt vector (ORG_*)
USARTTV:        equ     USART+3 ; Transmit interrupt vector (ORG_*)
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

;;; External data memory
        org     00H
rx_queue_size:  equ     32
rx_queue_buf:   ds      rx_queue_size
        org     USART
;;;  Software stack; pre-decrement, post-increment pointed by R1 on
;;;  external data memory
stack:          equ     $

;;; internal data memory
        org     BASE_MEMORY
rx_queue:       ds      queue_work_size

        org     ORG_RESET
        jmp     init
        org     ORG_INT
        jmp     isr_intr
init:
        mov     R1, #stack
        mov     R0, #rx_queue
        mov     R2, #rx_queue_buf
        mov     A, #rx_queue_size
        call    queue_init
init_usart:
        mov     R0, #USARTC
        clr     A
        movx    @R0, A
        movx    @R0, A
        movx    @R0, A          ; safest way to sync mode
        mov     A, #CMD_IR_bm
        movx    @R0, A          ; reset
        nop
        nop
        mov     A, #ASYNC_MODE
        movx    @R0, A
        nop
        nop
        mov     A, #RX_EN_TX_EN
        movx    @R0, A
        mov     A, #ORG_INT
        mov     R0, #USARTRV
        movx    @R0, A          ; enable RxRDY interrupt
        en      I

receive_loop:
        mov     R0, #rx_queue
        dis     I               ; Disable INT
        call    queue_remove
        en      I               ; Enable INT
        jnc     receive_loop
        jz      halt_to_system
        mov     R7, A           ; save letter
transmit_loop:
        mov     R0, #USARTS
        movx    A, @R0
        cpl     A
        jb      ST_TxRDY_bp, transmit_loop
transmit_data:
        mov     R0, #USARTD
        mov     A, R7
        movx    @R0, A
        xrl     A, #0DH
        jnz     receive_loop
        mov     R7, #0AH
        jmp     transmit_loop
halt_to_system:
        db      01H

        include "queue.inc"

isr_intr:
        sel     RB1             ; select register bank 1
        mov     R2, A           ; save A
isr_intr_receive:
        mov     R0, #USARTS
        movx    A, @R0
        cpl     A
        jb      ST_RxRDY_bp, isr_intr_recv_end
        mov     R0, #USARTD
        movx    A, @R0
        mov     R0, #rx_queue
        call    queue_add
isr_intr_recv_end:
        mov     A, R2
        retr
