;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     h8/500
        .maxmode on
;;; Status register
SR_T:           equ     $8000           ; Trace
SR_I_gm:        equ     $0700           ; Interrupt mask
CCR_N:          equ     %1000           ; Negative
CCR_Z:          equ     %0100           ; Zero
CCR_V:          equ     %0010           ; Overflow
CCR_C:          equ     %0001           ; Carry
;;; Exception vectors in maximum mode (Programming Manual Table 3-2):
;;; a word whose low byte is CP, then PC
VEC_RESET:      equ     $0000
VEC_INVALID:    equ     $0008           ; Invalid instruction
VEC_ZERODIV:    equ     $000C           ; DIVXU by zero
VEC_TRAPVS:     equ     $0010           ; TRAP/VS
VEC_ADDRERR:    equ     $0020           ; Address error
VEC_TRACE:      equ     $0024
VEC_NMI:        equ     $002C
VEC_TRAPA0:     equ     $0040
VEC_TRAPA15:    equ     $007C
VEC_IRQ0:       equ     $0080
;;; Interrupt priority register A, at the same address on every chip
IPRA:           equ     $FF00
IPRA_IRQ0_gm:   equ     %01110000       ; IRQ0 priority level
IPRA_IRQ0_1_gc: equ     %00010000       ; level 1
