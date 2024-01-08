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
        jp      init_config

        org     %1000
stack:  equ     $

init_config:
        srp     #%F0
        setrp   %F0
        ;; Stack is on external memory
        ld      P01M, #P01M_P0ADDR LOR P01M_P1DATA
        ld      P2M, #%FF       ; Port 2 is input
        ld      SPH, #HIGH stack
        ld      SPL, #LOW stack
        srp     #%10
        setrp   %10
        ld      r2, #HIGH rx_queue
        ld      r3, #LOW rx_queue
        ld      r1, #rx_queue_size
        call    queue_init

init_usart:
        ld      r12, #HIGH USARTC
        ld      r13, #LOW USARTC
        clr     r0
        lde     @rr12, r0
        lde     @rr12, r0
        lde     @rr12, r0       ; safest way to sync mode
        ld      r0, #CMD_IR_bm
        lde     @rr12, r0       ; reset
        nop
        nop
        ld      r0, #ASYNC_MODE
        lde     @rr12, r0       ; async 1stop 8data x16
        nop
        nop
        ld      r0, #RX_EN_TX_EN
        lde     @rr12, r0 ; RTS/DTR, error reset, Rx enable, Tx enable
        ld      r0, #INTR_IRQ0
        ld      r13, #LOW USARTRI
        lde     @rr12, r0 ; enable RxRDY interrupt using IRQ0
        ld      r0, #INTR_NONE
        ld      r13, #LOW USARTTI
        lde     @rr12, r0 ; disable TxRDY interrupt
        ld      r13, #LOW USARTS
        ld      r10, #HIGH USARTD
        ld      r11, #LOW USARTD

        ld      IPR, #IPR_BCA LOR IPR_B02 LOR IPR_C14 LOR IPR_A35
        ld      IMR, #IMR_IRQ0  ; enable IRQ0
        ei

receive_loop:
        di                      ; Disable INTR
        call    queue_remove
        ei                      ; Enable INTR
        jr      nc, receive_loop
        or      r0, r0
        jr      nz, transmit_loop
        halt
transmit_loop:
        lde     r1, @rr12       ; USARTC
        tm      r1, #ST_TxRDY_bm
        jr      z, transmit_loop
transmit_data:
        lde     @rr10, r0       ; USARTD
        cp      r0, #%0D
        jr      nz, receive_loop
        ld      r0, #%0A
        jr      transmit_loop

        include "queue.inc"

        setrp   -1
isr_intr:
        push    r0
        lde     r0, @rr12       ; USARTS
isr_intr_receive:
        tm      r0, #ST_RxRDY_bm
        jr      z, isr_intr_recv_end
        lde     r0, @rr10       ; USARTD
        call    queue_add
isr_intr_recv_end:
        pop     r0
        iret
