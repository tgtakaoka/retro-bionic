;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tlcs90
        include "tmp90c802.inc"

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jp      init

        org     ORG_SWI
        halt                    ; halt to system

        org     ORG_INTRX
        jp      isr_intr

P3CR_SET:       equ     P3CR_WAIT_ENB|P3CR_TXD|P3CR_RXD
SCMOD_SET:      equ     SCMOD_RXE|SCMOD_SM8|SCMOD_SCBAUD ; 8-bit
TRUN_SET:       equ     TRUN_BR9600|TRUN_PRRUN           ; 9600bps
SCCR_SET:       equ     SCCR_PE_DIS ; no parity, no handshake

        org     0100H
init:
        ld      sp, stack
        ld      ix, rx_queue
        ld      b, rx_queue_size
        call    queue_init
init_uart:
        ld      (P3CR), P3CR_SET
        ld      (SCMOD), SCMOD_SET
        ld      (TRUN), TRUN_SET
        ld      (SCCR), SCCR_SET
        set     INTEL_IERX_bp, (INTEL)
        ei

        ld      ix, rx_queue
receive_loop:
        di                      ; Disable INTR
        call    queue_remove
        ei                      ; Enable INTR
        jr      nc, receive_loop
        or      a, a
        jr      nz, transmit_data
        swi
transmit_data:
        ld      (SCBUF), a
        cp      a, 0DH
        jr      nz, receive_loop
        ld      a, 0AH
        jr      transmit_data

        include "queue.inc"

isr_intr:
        bit     IRFH_IRFRX_bp, (IRFH)
        jr      z, isr_intr_recv_end
        ld      a, (SCBUF)
        push    ix
        ld      ix, rx_queue
        call    queue_add
        pop     ix
isr_intr_recv_end:
        reti
