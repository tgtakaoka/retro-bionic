;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8051
        include "i8051.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     0FFF0H
USARTD:         equ     USART+0 ; Data register
USARTS:         equ     USART+1 ; Status register
USARTC:         equ     USART+1 ; Control register
USARTRV:        equ     USART+2 ; Receive interrupt vector (ORG_*)
USARTTV:        equ     USART+3 ; Transmit interrupt vector (ORG_*)
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

;;; External data memory
        org     2000H
rx_buffer_size:  equ     128
rx_buffer:
        ds      rx_buffer_size

;;; Internal data memory
        org     BASE_MEMORY
rx_queue:
        ds      queue_work_size
stack:  equ     $

        org     ORG_RESET
        ljmp    init
        org     ORG_IE0
        ljmp    isr_intr
        org     ORG_TF0
        ljmp    init
        org     ORG_IE1
        ljmp    init
        org     ORG_TF1
        ljmp    init
        org     ORG_RITI
        ljmp    init

init:
        mov     SP, #stack-1
        mov     R0, #rx_queue
        mov     R1, #rx_buffer_size
        mov     DPTR, #rx_buffer
        lcall   queue_init
init_uart:
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
        inc     DPTR
        mov     A, #ORG_IE0
        movx    @DPTR, A        ; enable Rx interrupt using INT0
        setb    IE.EX0          ; enable INT0
        setb    IE.EA           ; enable interrupt

receive_loop:
        mov     R0, #rx_queue
        clr     IE.EA           ; disable interrupt
        lcall   queue_remove
        setb    IE.EA           ; enable interrupt
        jnc     receive_loop
        jz      halt_to_system
        mov     R7, A           ; save character
transmit_loop:
        mov     DPTR, #USARTS
        movx    A, @DPTR
        jnb     ACC.ST_TxRDY_bp, transmit_loop
transmit_data:
        dec     DPL             ; DPTR=USARTD
        mov     A, R7
        movx    @DPTR, A
        cjne    A, #0DH, receive_loop
        mov     R7, #0AH
        sjmp    transmit_loop
halt_to_system:
        db      0A5H            ; halt to system

        include "queue.inc"

isr_intr:
        push    PSW
        clr     PSW.RS1
        setb    PSW.RS0         ; select BANK 1
        mov     R1, DPL
        mov     R2, DPH         ; save DPR
        mov     R3, A           ; save A
        mov     DPTR, #USARTS
        movx    A, @DPTR
        jnb     ACC.ST_RxRDY_bp, isr_intr_end
        dec     DPL             ; DPTR=USARTD
        movx    A, @DPTR
        mov     R0, #rx_queue
        lcall   queue_add
isr_intr_end:
        mov     A, R3           ; restore A
        mov     DPH, R2
        mov     DPL, R1         ; restore DPTR
        pop     PSW
        reti
