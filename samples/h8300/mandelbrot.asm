;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "hd6413238.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FF80
        include "../mc6800/mc6850.inc"

F:      equ     50              ; 1.0 in mandelbrot.inc's fixed point

rx_queue:       equ     $2000
rx_queue_size:  equ     128
tx_queue:       equ     $2080
tx_queue_size:  equ     128

stack:  equ     $1000

        org     VEC_RESET
        .data.w initialize

        org     VEC_IRQ0
        .data.w isr_irq

        org     $1000
initialize:
        mov.w   #stack, sp
        jsr     @init_io
        ;; #IRQ0 is low-level sensed from reset
        bset    #IER_IRQ0E_bp, @IER:8
        andc    #~CCR_I&$FF, ccr ; enable IRQ
loop:
        jsr     @mandelbrot
        jsr     @newline
        mov.w   #tx_queue, r1
        jsr     @queue_wait_empty
        bra     loop
halt_to_system:
        jsr     @@VEC_HALT      ; halt to system

        include "mandelbrot.inc"
        include "int16.inc"
        include "io.inc"

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
