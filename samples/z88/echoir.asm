;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z88.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     %FF00
USARTD:         equ     0       ; Receive/Transmit data
USARTS:         equ     1       ; Status register
USARTC:         equ     1       ; Control register
USARTRI:        equ     2       ; Receive interrupt name (IRQ0~2)
USARTTI:        equ     3       ; Transmit interrupt name (IRQ0~2)
        include "../z86/i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc LOR MODE_LEN8_gc LOR MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm LOR CMD_TxEN_bm

        org     %2000
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size

        org     %1000
stack:  equ     $

        org     VEC_IRQ0_P23
        dw      isr_intr

        org     ORG_RESET
        setrp   %C0
init_config:
        ld      EMT, #EMT_STACK_DM ; stack is on external data memory
        ld      P0M, #P0M_ADRH     ; Port 0 is address bus high
        ld      PM, #PM_P1BUS | PM_DM ; Port 1 is data bus and address bus low
        ldw     SPH, #stack
        ldw     RR2, #rx_queue
        ld      R1, #rx_queue_size
        call    queue_init

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
        ld      R0, #INTR_IRQ0
        lde     USARTRI(RR12), R0 ; enable RxRDY interrupt using IRQ0
        ld      R0, #INTR_NONE
        lde     USARTTI(RR12), R0 ; disable TxRDY interrupt

        ld      P2BM, #P2M_INTR_gm SHL 2 ; P23=input, interrupt enabled
        ld      IPR, #IPR_ABC ; (IRQ0 > IRQ1) > (IRQ2,3,4) > (IRQ5,6,7)
        ld      IMR, #IMR_IRQ0  ; enable IRQ0
        ei

receive_loop:
        di                      ; Disable INTR
        call    queue_remove
        ei                      ; Enable INTR
        jr      nc, receive_loop
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

        include "queue.inc"

        setrp   -1
isr_intr:
        ld      P2AIP, #1 SHL 5 ; clear P23 IRQ0
        push    R0
        lde     R0, USARTS(RR12)
isr_intr_receive:
        tm      R0, #ST_RxRDY_bm
        jr      z, isr_intr_recv_end
        lde     R0, USARTD(RR12)
        call    queue_add
isr_intr_recv_end:
        pop     R0
        iret
