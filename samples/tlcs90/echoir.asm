;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tlcs90
        include "tmp90c802.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     0FFF0H
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

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jp      init

        org     ORG_SWI
        halt                    ; halt to system

        org     ORG_INT0
        jp      isr_intr

        org     0100H
init:
        ld      sp, stack
        ld      ix, rx_queue
        ld      b, rx_queue_size
        call    queue_init
init_usart:
        ld      (USARTC), 0
        ld      (USARTC), 0
        ld      (USARTC), 0     ; safest way to sync mode
        ld      (USARTC), CMD_IR_bm
        nop
        nop
        ld      (USARTC), ASYNC_MODE
        nop
        nop
        ld      (USARTC), RX_EN_TX_EN
        ld      (USARTRV), ORG_INT0   ; enable Rx interrupt using INT0
        ld      (USARTTV), ORG_RESET  ; disable Tx interrupt
        set     INTEH_IE0_bp, (INTEH) ; Enable INT0
        ei

        ld      ix, rx_queue
receive_loop:
        di                      ; Disable INTR
        call    queue_remove
        ei                      ; Enable INTR
        jr      nc, receive_loop
        or      a, a
        jr      nz, transmit_loop
        swi
transmit_loop:
        bit     ST_TxRDY_bp, (USARTS)
        jr      z, transmit_loop
transmit_data:
        ld      (USARTD), a
        cp      a, 0DH
        jr      nz, receive_loop
        ld      a, 0AH
        jr      transmit_loop

        include "queue.inc"

isr_intr:
        bit     ST_RxRDY_bp, (USARTS)
        jr      z, isr_intr_recv_end
        ld      a, (USARTD)
        push    ix
        ld      ix, rx_queue
        call    queue_add
        pop     ix
isr_intr_recv_end:
        reti
