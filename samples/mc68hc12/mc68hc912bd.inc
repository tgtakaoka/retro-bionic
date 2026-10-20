;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC68HC912BD32
        include "mc68hc12.inc"
        include "mc68hc12_sci.inc"

VEC_SCI0:       equ     $FFD6  ; $FFD6: SCI0

;;; Internal memory, as reset leaves it in special expanded wide mode
REG_BASE:       equ     $0000  ; $0000-$01FF: registers
RAM_BASE:       equ     $0800  ; $0800-$0BFF: RAM
EEPROM_BASE:    equ     $0D00  ; $0D00-$0FFF: EEPROM

SCI0_BASE:      equ     REG_BASE+$C0

;;; Where the samples go: data and queues in RAM from $1000; code, and the
;;; stack below it, from $2000.
DATA_ORG:       equ     $1000
QUEUE_ORG:      equ     $1100
STACK_TOP:      equ     $2000   ; SP points at the last byte pushed
CODE_ORG:       equ     $2000
