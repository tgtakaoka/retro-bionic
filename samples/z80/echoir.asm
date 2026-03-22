;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z80.inc"
        include "usart.inc"

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
        ld      HL, ORG_RST38
        ld      (HL), 0FFH
        rst     38h

        include "queue.inc"

isr_intr:
        push    AF
        push    HL
        in      A, (USARTS)isr_intr_receive:
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
