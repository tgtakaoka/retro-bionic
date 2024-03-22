;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z86c
        option  "reg-alias", "disable"

        include "z8.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     %FF00
USARTD:         equ     USART+0 ; Receive/Transmit data
USARTS:         equ     USART+1 ; Srtatus register
USARTC:         equ     USART+1 ; Control register
USARTRI:        equ     USART+2 ; Receive interrupt name (IRQ0~2)
USARTTI:        equ     USART+3 ; Transmit interrupt name (IRQ0~2)
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc LOR MODE_LEN8_gc LOR MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm LOR CMD_TxEN_bm

        org     %2000
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size

        org     VEC_IRQ0
        dw      isr_intr

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
        ld      R2, #HIGH rx_queue
        ld      R3, #LOW rx_queue
        ld      R1, #rx_queue_size
        call    queue_init

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
        ld      R0, #INTR_IRQ0
        ld      R13, #LOW USARTRI
        lde     @RR12, R0 ; enable RxRDY interrupt using IRQ0
        ld      R0, #INTR_NONE
        ld      R13, #LOW USARTTI
        lde     @RR12, R0 ; disable TxRDY interrupt
        ld      R13, #LOW USARTS
        ld      R10, #HIGH USARTD
        ld      R11, #LOW USARTD

        ld      IPR, #IPR_BCA LOR IPR_B02 LOR IPR_C14 LOR IPR_A35
        ld      IMR, #IMR_IRQ0  ; enable IRQ0
        ei

receive_loop:
        di                      ; Disable INTR
        call    queue_remove
        ei                      ; Enable INTR
        jr      nc, receive_loop
        or      R0, R0
        jr      nz, transmit_loop
        halt
transmit_loop:
        lde     R1, @RR12       ; USARTC
        tm      R1, #ST_TxRDY_bm
        jr      z, transmit_loop
transmit_data:
        lde     @RR10, R0       ; USARTD
        cp      R0, #%0D
        jr      nz, receive_loop
        ld      R0, #%0A
        jr      transmit_loop

        include "queue.inc"

        setrp   -1
isr_intr:
        push    R0
        lde     R0, @RR12       ; USARTS
isr_intr_receive:
        tm      R0, #ST_RxRDY_bm
        jr      z, isr_intr_recv_end
        lde     R0, @RR10       ; USARTD
        call    queue_add
isr_intr_recv_end:
        pop     R0
        iret
