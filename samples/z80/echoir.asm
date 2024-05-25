;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z80
        include "z80.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     00H
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

        org     ORG_INT         ; mode 1
        jp      isr_intr

        org     0100H
init:
        ld      SP, stack
        ld      HL, rx_queue
        ld      B, rx_queue_size
        call     queue_init
init_usart:
        xor     A               ; clear A
        out     (USARTC), A
        out     (USARTC), A
        out     (USARTC), A     ; safest way to sync mode
        ld      A, CMD_IR_bm
        out     (USARTC), A     ; reset
        nop
        nop
        ld      A, ASYNC_MODE
        out     (USARTC), A
        nop
        nop
        ld      A, RX_EN_TX_EN
        out     (USARTC), A
        ld      A, ORG_INT
        out     (USARTRV), A    ; enable RxRDY interrupt using RST 7
        ld      A, ORG_RESET
        out     (USARTTV), A    ; disable TxRDY interrupt

        im      1
        ei

        ld      HL, rx_queue
receive_loop:
        di                      ; Disable INTR
        call    queue_remove
        ei                      ; Enable INTR
        jr      NC, receive_loop
        ld      B, A            ; save character
        or      A
        jr      Z, halt_to_system
transmit_loop:
        in      A, (USARTS)
        bit     ST_TxRDY_bp, A
        jr      Z, transmit_loop
transmit_data:
        ld      A, B
        out     (USARTD), A
        cp      0DH
        jr      NZ, receive_loop
        ld      B, 0AH
        jr      transmit_loop
halt_to_system:
        halt

        include "queue.inc"

isr_intr:
        push    AF
        push    HL
        in      A, (USARTS)
isr_intr_receive:
        bit     ST_RxRDY_bp, A
        jr      Z, isr_intr_recv_end
        in      A, (USARTD)
        ld      HL, rx_queue
        call    queue_add
isr_intr_recv_end:
        pop     HL
        pop     AF
        ei
        reti
