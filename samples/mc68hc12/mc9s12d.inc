;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC9S12DG128B
        include "mc68hc12.inc"
        include "mc68hc12_sci.inc"

VEC_SCI0:       equ     $FFD6  ; $FFD6: SCI0

;;; Internal memory, as reset leaves it in special test mode
REG_BASE:       equ     $0000  ; $0000-$03FF: registers
RAM_BASE:       equ     $0000  ; $0000-$1FFF: RAM, under the registers

SCI0_BASE:      equ     REG_BASE+$C8

;;; Where the samples go, the same on every part: data, queues and the
;;; stack from $1000, inside or out; code from $4000, outside on all.
DATA_ORG:       equ     $1000
QUEUE_ORG:      equ     $1100
STACK_TOP:      equ     $2000   ; SP points at the last byte pushed
CODE_ORG:       equ     $4000
