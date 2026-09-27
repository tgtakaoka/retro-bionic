;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Cache instructions and data. The debugger caches nothing after reset;
;;; a program that wants the cache turns it on itself.
        ld      C, CNTL_CACHE
        ld      HL, 0           ; M/C=0 I=0 D=0
        ldctl   (C), HL
        pcache
