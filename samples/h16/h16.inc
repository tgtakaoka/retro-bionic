;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     h16
;;; Exception vectors, offsets from EBR (User's Manual Table 4-13)
VEC_RESET:      equ     $000            ; SSP, then PC
VEC_BUSERR:     equ     $008
VEC_ILLEGAL:    equ     $010
VEC_ZERODIV:    equ     $014
VEC_TRAP:       equ     $01C
VEC_PRIV:       equ     $020
VEC_TRACE:      equ     $024
VEC_IRQ1:       equ     $074
VEC_IRQ0:       equ     $078
VEC_NMI:        equ     $07C
VEC_TRAPA0:     equ     $080
VEC_TRAPA15:    equ     $0BC
