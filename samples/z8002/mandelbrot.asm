;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z8002.inc"
        include "usart.inc"

stack:  equ     1000H           ; system mode
ustack: equ     3000H           ; normal mode

        org     ORG_RESET
        word    0
        word    FCW_SN          ; system mode, interrupts disabled
        word    init

        org     PSA_SC
        word    FCW_SN          ; I/O for the program in normal mode
        word    isr_sc

        org     PSA_VI
        word    FCW_SN          ; the handlers run with interrupts disabled
        org     PSA_VI+2+2*RX_ID
        word    isr_rx
        org     PSA_VI+2+2*TX_ID
        word    isr_tx

        org     0100H
init:
        ld      r15, #stack
        clr     r0
        ldctl   psap, r0        ; the Program Status Area at 0000H
        ld      r0, #ustack
        ldctl   nsp, r0
        call    init_io
        ldps    user_ps         ; on in normal mode, which does I/O by SC

loop:
        call    mandelbrot
        sc      #SC_NEWLINE
        jr      loop

user_ps:
        word    FCW_VIE         ; normal mode, vectored interrupts enabled
        word    loop

        include "mandelbrot.inc"
        include "io.inc"

;;; Return from an interrupt handler.
isr_return:
        iret
