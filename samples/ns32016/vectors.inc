;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Reset starts at 0 in supervisor mode with PSR 0. The one module has
;;; its program base at 0, so a procedure's offset is its address; SB
;;; and the link base are unused.
        org     0
        br      init

        org     0x10
module: .double 0               ; static base
        .double 0               ; link base
        .double 0               ; program base
        .double 0
