;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8086
        include "usart.inc"

        org     VEC_RX*4
        dw      isr_rx, 0
        dw      isr_tx, 0

        org     1000H
stack:  equ     $

init:
        xor     ax, ax
        mov     ds, ax
        mov     ss, ax
        mov     sp, stack
        call    init_io
        sti

        include "echoitr.inc"
        include "io.inc"

        org     0FFFF0H
        jmpf    0:init

        end
