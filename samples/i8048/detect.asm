;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     msm80c39
        org     0
        mov     A, #1
        mov     R0, A
        mov     R1, A
        dec     @R0
        nop
        mov     A, R1
        movx    @R0, A
        halt
