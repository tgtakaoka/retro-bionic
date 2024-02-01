;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "mc6805.inc"
;;; MC146805E
;;; Vector
VEC_IRQ:        equ     $1FFA  ; $1FFA: Maskable Interrupt Request
VEC_SWI:        equ     $1FFC  ; $1FFC: Software Interrupt
VEC_RESET:      equ     $1FFE  ; $1FFE: Reset
