;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; MC6800/MC6802/MC6808
;;; Condition Code Register (CC)
CC_CARRY      equ  %00000001   ; set to 1 if carry occurred
CC_OVERFLOW   equ  %00000010   ; set to 1 if overflow occurred
CC_ZERO       equ  %00000100   ; set to 1 if result is zero
CC_NEGATIVE   equ  %00001000   ; set to 1 if result is negative
CC_IRQ        equ  %00010000   ; if 1, IRQ is masked
CC_HALF_CARRY equ  %00100000   ; if 1, decimal carry from least digit occurred

;;; Vector
VEC_IRQ:        equ     $FFF8  ; $FFF8: Maskable Interrupt Request
VEC_SWI:        equ     $FFFA  ; $FFFA: Software Interrupt
VEC_NMI:        equ     $FFFC  ; $FFFC: Non Maskable Interrupt
VEC_RESET:      equ     $FFFE  ; $FFFE: Reset
