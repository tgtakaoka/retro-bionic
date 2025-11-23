;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "kl5c80a12.inc"
        include "../z80/usart.inc"

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jp      init

        org     0080H
vector:
        dw      isr_intr        ; IR0

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

        ld      a, 1            ; enable interrupt on USART
        out     (USARTRV), a
        ei
        ld      a, 1            ; select edge trigger
        out     (LERL), a
        ld      a, high vector
        ld      i, a
        ld      a, low vector
        out     (IVR), a        ; vector register
        ld      a, ~1           ; disable mask for IR0
        out     (IMRL), a       ;
        im      2               ; mode 2 only

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

        include "../z80/queue.inc"

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
