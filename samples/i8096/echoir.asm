;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "i8096.inc"
        include "usart.inc"

        org     3000H
rx_queue_size:  equ     128
rx_queue:       dsb     rx_queue_size

        org     1000H
stack:  equ     $

        org     VEC_EXTINT
        dcw     isr_intr

        org     1AH
A:      dsb     1
B:      dsb     1
HL:     dsw     1
DE:     dsw     1

        org     ORG_RESET
init:
        ld      SP, #stack
        ldb     INT_MASK, INT_EXTINT ; enable EXTINT


        ld      HL, #rx_queue
        ldb     B, #rx_queue_size
        scall   queue_init
init_usart:
        stb     ZERO, USARTC
        stb     ZERO, USARTC
        stb     ZERO, USARTC    ; safest way to sync mode
        ldb     A, #CMD_IR_bm
        stb     A, USARTC       ; reset
        nop
        nop
        ldb     A, #ASYNC_MODE
        stb     A, USARTC
        nop
        nop
        ldb     A, #RX_EN_TX_EN
        stb     A, USARTC
        ldb     A, #1
        stb     A, USARTRV      ; enable receive interrupt
        ei

        ld      HL, #rx_queue
receive_loop:
        di                      ; Disable INTR
        scall   queue_remove
        ei                      ; Enable INTR
        jnc     receive_loop
        ldb     B, A            ; save character
        or      A, A
        je      halt_to_system
transmit_loop:
        ldb     A, USARTS
        jbc     A, ST_TxRDY_bp, transmit_loop
transmit_data:
        stb     B, USARTD
        cmpb    B, #0DH
        jne     receive_loop
        ldb     B, #0AH
        sjmp    transmit_loop

halt_to_system:
        ld      A, #VEC_TRAP
        st      A, VEC_TRAP     ; halt to system
        trap

        include "queue.inc"

isr_intr:
        pushf
        push    A               ; push A,B
        push    HL
        ldb     A, USARTS
        jbc     A, ST_RxRDY_bp, isr_intr_recv_end
        ldb     A, USARTD
        ld      HL, #rx_queue
        scall   queue_add
isr_intr_recv_end:
        pop     HL
        pop     A
        popf
        ret
