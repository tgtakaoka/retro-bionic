;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; samples/arith/fmandel.c in Z8070 instructions, which Zilog's
;;; emulator, FPE.O of CP/M-8000, runs: `make fmandel` links it in at
;;; 5000H, above DATA, into fmandel.hex.
        include "z8002.inc"
        fpu     on
        include "usart.inc"
        include "fpesys.inc"

stack:  equ     1000H           ; system mode
ustack: equ     3000H           ; normal mode
;;; The Program Status Area in a page of its own: at 0000H the extended
;;; instruction trap's FCW would be the reset PC, at 0004H.
PSA:    equ     0100H

        org     ORG_RESET
        word    0
        word    FCW_SN          ; system mode, interrupts disabled
        word    init

        org     PSA+PSA_EPA
        word    FCW_SN          ; FPE runs with interrupts disabled
        word    fpe_trap

        org     PSA+PSA_SC
        word    FCW_SN          ; I/O for the program in normal mode
        word    isr_sc

        org     PSA+PSA_VI
        word    FCW_SN          ; the handlers run with interrupts disabled
        org     PSA+PSA_VI+2+2*RX_ID
        word    isr_rx
        org     PSA+PSA_VI+2+2*TX_ID
        word    isr_tx

        org     0200H
init:
        ld      sp, #stack
        ld      r0, #PSA
        ldctl   psap, r0
        ld      r0, #ustack
        ldctl   nsp, r0
        call    init_io
        ldps    user_ps         ; on in normal mode, which does I/O by SC

start:
        clr     r0
        clr     r1
        fldctl  fctl5, r0, #1   ; the emulator's control registers, as
        fldctl  fctl4, r0, #2   ; DRI's C start-up sets them
loop:
        call    fmandel
        sc      #SC_NEWLINE
        ld      r1, #tx_queue
        call    queue_wait_empty
        jr      loop

user_ps:
        word    FCW_VIE         ; normal mode, vectored interrupts enabled
        word    start

        include "fmandel8070.inc"
        include "fpe.inc"
        include "io.inc"
        include "queue_wait.inc"

;;; Return from an interrupt handler.
isr_return:
        iret
