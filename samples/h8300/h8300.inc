;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     h8/300
;;; Condition code register
CCR_I:          equ     %10000000       ; Interrupt mask
CCR_H:          equ     %00100000       ; Half carry
CCR_N:          equ     %00001000       ; Negative
CCR_Z:          equ     %00000100       ; Zero
CCR_V:          equ     %00000010       ; Overflow
CCR_C:          equ     %00000001       ; Carry

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
