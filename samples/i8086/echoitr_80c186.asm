;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; On the BionicN80C186/N80C187 boards: INT0 in cascade mode, and an
;;; End Of Interrupt after each handler.
        cpu     80186
        include "usart.inc"

        org     VEC_RX*4
        dw      isr_rx_eoi, 0
        dw      isr_tx_eoi, 0

        org     1000H
stack:  equ     $

init:
        xor     ax, ax
        mov     ds, ax
        mov     ss, ax
        mov     sp, stack
        call    init_io
        call    init_icu
        sti

        include "echoitr.inc"
        include "io.inc"

        align   2
isr_rx_eoi:
        pushf
        callf   0:isr_rx
        jmp     isr_eoi

        align   2
isr_tx_eoi:
        pushf
        callf   0:isr_tx
        jmp     isr_eoi

        include "icu_80c186.inc"

        org     0FFFF0H
        jmpf    0:init

        end
