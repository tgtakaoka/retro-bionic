;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Z8
        cpu     Z8
;;;
FLAGS:  equ     252               ; R/W: Flags register
F_CARRY:        equ     %(2)10000000 ; set to 1 if carry
F_ZERO:         equ     %(2)01000000 ; set to 1 if zero
F_SIGN:         equ     %(2)00100000 ; set to 1 if negative
F_OVERFLOW:     equ     %(2)00010000 ; set to 1 if overflow
F_DECIMAL_ADJ:  equ     %(2)00001000 ; decimal adjust
F_HALF_CARRY:   equ     %(2)00000100 ; set to 1 if carry from bit-3
F_USER2:        equ     %(2)00000010 ; User flag F2
F_USER1:        equ     %(2)00000001 ; User flah F1
