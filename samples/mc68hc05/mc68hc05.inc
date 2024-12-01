;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC68HC05
        cpu     68hc05
;;; Condition Code Register (CC)
CC_CARRY      equ  %00000001   ; set to 1 if carry occurred
CC_ZERO       equ  %00000010   ; set to 1 if result is zero
CC_NEGATIVE   equ  %00000100   ; set to 1 if result is negative
CC_IRQ        equ  %00001000   ; if 1, IRQ is masked
CC_HALF_CARRY equ  %00010000   ; if 1, decimal carry from least digit occurred
