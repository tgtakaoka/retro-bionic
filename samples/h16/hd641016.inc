;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "h16.inc"
;;; HD641016 on-chip registers, I/O base IBR H'FF at reset
MCR:            equ     $FFFFF8         ; Memory control register
AWCR0:          equ     $FFFF2C         ; Area 0 wait control register
IPR2:           equ     $FFFF44         ; Interrupt priority register 2
IR0P_gm:        equ     %00110000       ; IRQ0 priority level
IR0P_LOW_gc:    equ     %00010000       ; level 1
ICR:            equ     $FFFF46         ; Interrupt control register
