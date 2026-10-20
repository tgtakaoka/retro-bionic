;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC9S12XDP512
        include "../mc68hc12/mc68hc12.inc"
        include "../mc68hc12/mc68hc12_sci.inc"
        cpu     68hcs12x

VEC_SCI0:       equ     $FFD6  ; $FFD6: SCI0

;;; Internal memory, as reset leaves it in special test mode with ROMCTL
;;; low: the Flash off
REG_BASE:       equ     $0000  ; $0000-$07FF: registers
EEPROM_BASE:    equ     $0800  ; $0800-$0FFF: EEPROM
RAM_BASE:       equ     $1000  ; $1000-$3FFF: RAM

SCI0_BASE:      equ     REG_BASE+$C8

;;; Where the samples go: data, queues and the stack in internal RAM; code
;;; outside, from $4000.
DATA_ORG:       equ     $2000
QUEUE_ORG:      equ     $2100
STACK_TOP:      equ     $4000   ; SP points at the last byte pushed
CODE_ORG:       equ     $4000
