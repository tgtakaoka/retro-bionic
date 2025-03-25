;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "mc68hc08.inc"
;;; MC68HC08AZ0
RAM_START:      equ     $0040   ; Internal RAM $0040-$023F
STACK_TOP:      equ     $00FF   ; Stack $00FF->$0040

;;; COP
COP_RESET:      equ     $FFFF
