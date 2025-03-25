;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "mc68hc08.inc"
;;; MC68HC08AZ0
RAM_START:      equ     $0050   ; Internal RAM $0050-$044F
STACK_TOP:      equ     $00FF   ; Stack $00FF->$0050

;;; COP
COP_RESET:      equ     $FFFF
