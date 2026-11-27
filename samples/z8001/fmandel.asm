;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; samples/arith/fmandel.c in Z8070 instructions, which Zilog's
;;; emulator, FPE.O of CP/M-8000, runs: `make fmandel` links it in at
;;; <<0>>5000H into fmandel.hex.
        cpu     z8001
        include "z8001.inc"
        fpu     on
        include "../z8002/usart.inc"
        include "../z8002/fpesys.inc"
DATA:           equ     SEGDATA
FPE_STACK:      equ     00001000H ; RR14: segment 0, with FPE

        org     ORG_RESET
        word    0
        word    FCW_SEG|FCW_SN  ; segmented system mode, interrupts disabled
        word    CODE_SEG, init-CODE

        org     PSA_EPA
        word    0
        word    FCW_SEG|FCW_SN  ; FPE runs with interrupts disabled
        word    0, fpe_trap

        org     PSA_SC
        word    0
        word    FCW_SEG|FCW_SN  ; I/O for the program in normal mode
        word    CODE_SEG, isr_sc-CODE

        org     PSA_VI
        word    0
        word    FCW_SEG|FCW_SN  ; the handlers run with interrupts disabled
        org     PSA_VI+4+2*RX_ID
        word    CODE_SEG, isr_rx-CODE
        org     PSA_VI+4+2*TX_ID
        word    CODE_SEG, isr_tx-CODE

        org     0100H
        include "fpe.inc"

        cpu     z8001
        org     CODE+0100H
init:
        ldl     sp, #FPE_STACK
        clr     r0
        ldctl   psapseg, r0
        ldctl   psapoff, r0     ; the Program Status Area at <<0>>0000H
        ld      r0, #USTACK_SEG
        ldctl   nspseg, r0
        ld      r0, #USTACK_OFF
        ldctl   nspoff, r0
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
        word    0
        word    FCW_SEG|FCW_VIE ; normal mode, vectored interrupts enabled
        word    CODE_SEG, start-CODE

        include "../z8002/fmandel8070.inc"
        include "../z8002/io.inc"
        include "../z8002/queue_wait.inc"

;;; Return from an interrupt handler.
isr_return:
        iret
