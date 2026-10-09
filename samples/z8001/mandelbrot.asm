;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z8001
        include "z8001.inc"
        include "../z8002/usart.inc"

        org     ORG_RESET
        word    0
        word    FCW_SEG|FCW_SN  ; segmented system mode, interrupts disabled
        word    CODE_SEG, init-CODE

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

        org     CODE+0100H
init:
        ldl     rr14, #STACK
        clr     r0
        ldctl   psapseg, r0
        ldctl   psapoff, r0     ; the Program Status Area at <<0>>0000H
        ld      r0, #USTACK_SEG
        ldctl   nspseg, r0
        ld      r0, #USTACK_OFF
        ldctl   nspoff, r0
        call    init_io
        ldps    user_ps         ; on in normal mode, which does I/O by SC

loop:
        call    mandelbrot
        sc      #SC_NEWLINE
        jr      loop

user_ps:
        word    0
        word    FCW_SEG|FCW_VIE ; normal mode, vectored interrupts enabled
        word    CODE_SEG, loop-CODE

        include "mandelbrot.inc"
        include "io.inc"

;;; Return from an interrupt handler.
isr_return:
        iret
