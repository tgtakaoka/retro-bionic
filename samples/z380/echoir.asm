;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z380.inc"
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
        ld      IX, rx_queue
        ld      B, rx_queue_size
        call     queue_init
init_usart:
        xor     A               ; clear A
        outa    (USARTC), A
        outa    (USARTC), A
        outa    (USARTC), A     ; safest way to sync mode
        ld      A, CMD_IR_bm
        outa    (USARTC), A     ; reset
        nop
        nop
        ld      A, ASYNC_MODE
        outa    (USARTC), A
        nop
        nop
        ld      A, RX_EN_TX_EN
        outa    (USARTC), A
        ld      A, ORG_INT
        outa    (USARTRV), A    ; enable RxRDY interrupt using RST 7
        ld      A, ORG_RESET
        outa    (USARTTV), A    ; disable TxRDY interrupt

        im      1
        ei

        ld      IX, rx_queue
receive_loop:
        di                      ; Disable INTR
        call    queue_remove
        ei                      ; Enable INTR
        jr      NC, receive_loop
        ld      B, A            ; save character
        or      A
        jr      Z, halt_to_system
transmit_loop:
        ina     A, (USARTS)
        bit     ST_TxRDY_bp, A
        jr      Z, transmit_loop
transmit_data:
        ld      A, B
        outa    (USARTD), A
        cp      0DH
        jr      NZ, receive_loop
        ld      B, 0AH
        jr      transmit_loop
halt_to_system:
        ld      A, 0FFH
        ld      (ORG_RST38), A
        rst     38h

        include "queue.inc"

isr_intr:
        ex      AF, AF'
        exall                   ; the ISR's own BC, DE, HL, IX and IY
        ina     A, (USARTS)
isr_intr_receive:
        bit     ST_RxRDY_bp, A
        jr      Z, isr_intr_recv_end
        ina     A, (USARTD)
        ld      IX, rx_queue
        call    queue_add
isr_intr_recv_end:
        exall
        ex      AF, AF'
        ei
        reti
