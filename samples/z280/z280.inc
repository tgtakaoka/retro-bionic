;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Z280
        cpu     z280
;;; Reset origin
ORG_RESET:      equ     0000H
;;; Control register
CNTL_MSR:       equ     00H     ; Master Status register
CNTL_ISR:       equ     16H     ; Interrupt Status register
CNTL_VTABLE:    equ     06H     ; Interrupt/Trap Vector Table Pointer
CNTL_IOPAGE:    equ     08H     ; I/O Page register
CNTL_BUSINIT:   equ     0FFH    ; Bus Timing and Initialization register
CNTL_BUSCNTL:   equ     02H     ; Bus Timing and Control register
CNTL_SPLIMIT:   equ     04H     ; Stack Limit register
CNTL_TRAP:      equ     10H     ; Trap Control register
CNTL_CACHE:     equ     12H     ; Cach Control register
CNTL_LOCAL:     equ     14H     ; Local Address register
;;; On-Chip Peripherals
REFRESH_RATE:   equ     0FF00E8H ; Refresh Rate Reigster
