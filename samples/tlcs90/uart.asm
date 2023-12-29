;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tlcs90
        include "tmp90c802.inc"

        org     1000H
stack:  equ     $

        org     ORG_RESET
        ld      sp, stack
        jp      init_uart

        org     ORG_SWI
        halt                    ; halt to system

P3CR_SET:       equ     P3CR_WAIT_ENB|P3CR_TXD|P3CR_RXD
SCMOD_SET:      equ     SCMOD_RXE|SCMOD_SM8|SCMOD_SCBAUD ; 8-bit
TRUN_SET:       equ     TRUN_BR9600|TRUN_PRRUN           ; 9600bps
SCCR_SET:       equ     SCCR_PE_DIS ; no parity, no handshake

        org     0100H
init_uart:
        ld      (P3CR), P3CR_SET
        ld      (SCMOD), SCMOD_SET
        ld      (TRUN), TRUN_SET
        ld      (SCCR), SCCR_SET

receive_loop:
        bit     IRFH_IRFRX_bp, (IRFH)
        jr      z, receive_loop
receive_data:
        ld      a, (SCBUF)
        or      a, a
        jr      nz, transmit_data
        swi
transmit_data:
        ld      (SCBUF), a
        cp      a, 0DH
        jr      nz, receive_loop
        ld      a, 0AH
        jr      transmit_data
