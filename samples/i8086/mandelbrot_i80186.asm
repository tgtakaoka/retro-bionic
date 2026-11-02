;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; The mandelbrot with 80186 instructions, on the P8086 board's V20/V30.
        cpu     80186
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

loop:
        call    mandelbrot
        call    newline
        mov     bx, tx_queue
wait_tx:
        cmp     byte ptr [bx+queue_len], 0
        jne     wait_tx         ; until the frame is sent
        jmp     loop

        include "../i80186/mandelbrot.inc"
        include "io.inc"

        org     0FFFF0H
        jmpf    0:init

        end
