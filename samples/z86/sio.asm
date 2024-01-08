;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z86c
        option  "reg-alias", "disable"

        include "z8.inc"

        org     ORG_RESET
        jp      init_config

        org     %1000
stack:  equ     $

init_config:
        srp     #%F0
        setrp   %F0
        ;; Stack is on external memory
        ld      P01M, #P01M_P0ADDR LOR P01M_P1DATA
        ld      SPH, #HIGH stack
        ld      SPL, #LOW stack
        srp     #%10
        setrp   %10

;;; XTAL=14.7546MHz
;;; p=1 for PRE0, t=12 for T0
;;; bit rate = 14754600 / (2 x 4 x p x t x 16) = 9600 bps
init_sio:
        ld      r0, SIO          ; dummy read
        or      PORT3, #%80      ; TxD(P37)=High
        ld      P3M, #P3M_SERIAL ; enable SIO I/O
        ld      T0, #12
        ld      PRE0, #(1 SHL PRE0_gp) LOR PRE0_MODULO ; modulo-N
        or      TMR, #TMR_LOAD_T0 LOR TMR_ENABLE_T0

init_irq:
        ld      IPR, #IPR_ACB LOR IPR_A35 LOR IPR_C41 LOR IPR_B02
        clr     IMR            ; mask all IRQs
        ei                     ; clear IRQ and enable interrupt system
        di                     ; disable IRQ heading
        or      IRQ, #IRQ_IRQ4

receive_loop:
        tm      IRQ, #IRQ_IRQ3  ; check IRQ3
        jr      z, receive_loop
        ld      r0, SIO
        and     IRQ, #LNOT IRQ_IRQ3 ; clear IRQ3
        or      r0, r0
        jr      nz, transmit_loop
        halt
transmit_loop:
        tm      IRQ, #IRQ_IRQ4  ; check IRQ4
        jr      z, transmit_loop
transmit_data:
        ld      SIO, r0
        and     IRQ, #LNOT IRQ_IRQ4 ; clear IRQ4
        cp      r0, #%0D
        jr      nz, receive_loop
        ld      r0, #%0A
        jr      transmit_loop
