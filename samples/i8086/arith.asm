;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8086
        include "usart.inc"

        org     0100H
stack:  equ     1000H

init:
        xor     ax, ax
        mov     ds, ax
        mov     ss, ax
        mov     sp, stack
        mov     ah, RX_EN_TX_EN
        call    init_usart

        call    arith
halt_to_system:
        hlt

        include "arith.inc"
        include "init_usart.inc"

        org     0FFFF0H
        jmpf    0:init

        end
