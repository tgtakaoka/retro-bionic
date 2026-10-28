;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8086
        include "usart.inc"

        org     1000H
stack:  equ     $

init:
        xor     ax, ax
        mov     ds, ax
        mov     ss, ax
        mov     sp, stack
        mov     ah, RX_EN_TX_EN
        call    init_usart

        include "echo.inc"
        include "init_usart.inc"

        org     0FFFF0H
        jmpf    0:init

        end
