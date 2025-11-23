;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "../mc6809/mc6809.inc"
        cpu     HD6309
;;; Mode and Error Register (MD)
MD_EMULATION  equ  %00000001   ; if 1, 6309 native mode
MD_FIRQ_MODE  equ  %00000010   ; if 1, FIRQ operate as IRQ
MD_ILLEGAL_OP equ  %01000000   ; set to 1 if illegal instruction occurred
MD_DIV_ZERO   equ  %10000000   ; set to 1 if divided by 0 occurred

;;; Vector
VEC_DIVZERO:    equ     $FFF0   ; $FFF0: Divided by Zero Trap
VEC_ILLOPC:     equ     $FFF0   ; $FFF0: Illegal Opcode Trap
