;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC9S12XDP512
        include "../mc68hc12/mc68hc12.inc"
        cpu     68hcs12x

;;; Internal memory, as reset leaves it in emulation expanded mode with
;;; emulation memory in place of the Flash
REG_BASE:       equ     $0000  ; $0000-$07FF: registers
EEPROM_BASE:    equ     $0800  ; $0800-$0FFF: EEPROM
RAM_BASE:       equ     $1000  ; $1000-$3FFF: RAM

;;; Where the samples go, as for the CPU12 ones: data, queues and the
;;; stack in internal RAM from $1000; code outside, from $4000.
DATA_ORG:       equ     $1000
QUEUE_ORG:      equ     $1100
STACK_TOP:      equ     $2000   ; SP points at the last byte pushed
CODE_ORG:       equ     $4000
