;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP1600
        cpu     cp1600
        radix   10
;;; Reset address (see debugger InstCp1610::RESET_PC)
RESET_PC:       equ     X'1000'
