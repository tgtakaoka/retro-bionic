;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; The i8080 mandelbrot, unchanged, on a V20/V30 in 8080 emulation mode.
;;; The Makefile appends ../i8080/mandelbrot.hex to this image at
;;; SEG_I8080:0000. Its USART interrupts, types RST 5 and RST 6, leave
;;; emulation for the native handlers below, which serve its queues
;;; through DS0; IRET resumes it.
        include "../i8080/i8080.inc"
        cpu     V30
        include "usart.inc"

SEG_I8080:      equ     1000H   ; the 8080 program's code and data
VEC_I8080:      equ     22H     ; BRKEM's vector to its reset entry

        org     INST_RST5*4
        dw      isr_rx, 0
        org     INST_RST6*4
        dw      isr_tx, 0
        org     VEC_I8080*4
        dw      ORG_RESET, SEG_I8080

        org     1000H
stack:  equ     $

init:
        mov     ax, SEG_I8080
        mov     ds, ax          ; DS0, the 8080 data segment
        xor     ax, ax
        mov     ss, ax
        mov     sp, stack
        brkem   VEC_I8080
        jmp     $               ; the 8080 program never returns

        include "io.inc"

        org     0FFFF0H
        jmpf    0:init

        end
