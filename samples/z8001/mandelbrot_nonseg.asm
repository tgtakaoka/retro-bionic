;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z8002           ; nonsegmented: the Z8002's code
        include "z8001.inc"
        include "../z8002/usart.inc"

stack:  equ     1000H           ; system mode
ustack: equ     3000H           ; normal mode

        org     ORG_RESET
        word    0
        word    FCW_SN          ; system mode, interrupts disabled
        word    0               ; segment 0
        word    init

        org     PSA_SC
        word    0
        word    FCW_SN          ; I/O for the program in normal mode
        word    0, isr_sc

        org     PSA_VI
        word    0
        word    FCW_SN          ; the handlers run with interrupts disabled
        org     PSA_VI+4+2*RX_ID
        word    0, isr_rx
        org     PSA_VI+4+2*TX_ID
        word    0, isr_tx

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

        include "../z8002/mandelbrot.inc"
        include "../z8002/io.inc"

;;; Return from an interrupt handler. The Z8001 must be segmented to IRET
;;; (its operation in nonsegmented mode is undefined), and then pops the
;;; frame through RR14, the system stack's segment the debugger zeroes
;;; at reset.
isr_return:
        push    @r15, r1
        ldctl   r1, fcw
        set     r1, #15         ; FCW_SEG
        ldctl   fcw, r1
        word    97E1H           ; pop r1, @rr14
        iret
