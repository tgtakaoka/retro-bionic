;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "hd6413238.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FF80
        include "../mc6800/mc6850.inc"

stack:  equ     $1000

        org     VEC_RESET
        .data.w initialize

        org     $1000
initialize:
        mov.w   #stack, sp
        jsr     @init_acia
        include "echo.inc"
halt_to_system:
        jsr     @@VEC_HALT      ; halt to system

        include "acia.inc"

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
