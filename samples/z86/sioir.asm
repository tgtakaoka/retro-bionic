;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z86c
        option  "reg-alias", "disable"

        include "z8.inc"

        org     %2000
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size

        org     VEC_IRQ3
        dw      isr_intr

        org     ORG_RESET
        setrp   -1
        jp      init_config

        org     %1000
stack:  equ     $

init_config:
        ;; Stack is on external memory
        ld      P01M, #P01M_P0ADDR LOR P01M_P1DATA
        ld      SPH, #HIGH stack
        ld      SPL, #LOW stack
        srp     #%10
        setrp   %10
        ld      R2, #HIGH rx_queue
        ld      R3, #LOW rx_queue
        ld      R1, #rx_queue_size
        call    queue_init

;;; XTAL=14.7546MHz
;;; p=1 for PRE0, t=12 for T0
;;; bit rate = 14754600 / (2 x 4 x p x t x 16) = 9600 bps
init_sio:
        ld      R0, SIO          ; dummy read
        or      PORT3, #%80      ; TxD(P37)=High
        ld      P3M, #P3M_SERIAL ; enable SIO I/O
        ld      T0, #12
        ld      PRE0, #(1 SHL PRE0_gp) LOR PRE0_MODULO ; modulo-N
        or      TMR, #TMR_LOAD_T0 LOR TMR_ENABLE_T0

init_irq:
        ld      IPR, #IPR_ACB LOR IPR_A35 LOR IPR_C41 LOR IPR_B02
        ld      IMR, #IMR_IRQ3 ; enable IRQ3
        ei                     ; clear IRQ and enable interrupt system
        or      IRQ, #IRQ_IRQ4

receive_loop:
        di                      ; Disable INTR
        call    queue_remove
        ei                      ; Enable INTR
        jr      nc, receive_loop
        or      R0, R0
        jr      nz, transmit_loop
        halt
transmit_loop:
        tm      IRQ, #IRQ_IRQ4  ; check IRQ4
        jr      z, transmit_loop
transmit_data:
        ld      SIO, R0
        and     IRQ, #LNOT IRQ_IRQ4 ; clear IRQ4
        cp      R0, #%0D
        jr      nz, receive_loop
        ld      R0, #%0A
        jr      transmit_loop

        include "queue.inc"

        setrp   -1
isr_intr:
        push    R0
        ld      R0, SIO
        and     IRQ, #LNOT IRQ_IRQ3
        push    R3
        push    R2
        ld      R2, #HIGH rx_queue
        ld      R3, #LOW rx_queue
        call    queue_add
        pop     R2
        pop     R3
        pop     R0
        iret
