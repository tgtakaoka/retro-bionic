;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "mc68hc16.inc"
;;; MC68HC16Z
;;; Module registers, in bank F with MM=1 as reset leaves it (User's
;;; Manual Table D-1); reach them with EK, XK, YK or ZK = REG_BANK.
REG_BANK:       equ     $F
ADC_BASE:       equ     $F700  ; $F700: Analog-to-Digital Converter
MRM_BASE:       equ     $F820  ; $F820: Masked ROM Module (Z2, Z3)
GPT_BASE:       equ     $F900  ; $F900: General-Purpose Timer
QSM_BASE:       equ     $FC00  ; $FC00: Queued Serial Module
MCCI_BASE:      equ     $FC00  ; $FC00: Multichannel Communication Interface (Z4)

;;; System Integration Module (Table D-2)
SIMCR:          equ     $FA00  ; $FA00: SIM Module Configuration
SYNCR:          equ     $FA04  ; $FA04: Clock Synthesizer Control
RSR:            equ     $FA07  ; $FA07: Reset Status
PORTE0:         equ     $FA11  ; $FA11: Port E Data 0
PORTE1:         equ     $FA13  ; $FA13: Port E Data 1
DDRE:           equ     $FA15  ; $FA15: Port E Data Direction
PEPAR:          equ     $FA17  ; $FA17: Port E Pin Assignment
PORTF0:         equ     $FA19  ; $FA19: Port F Data 0
PORTF1:         equ     $FA1B  ; $FA1B: Port F Data 1
DDRF:           equ     $FA1D  ; $FA1D: Port F Data Direction
PFPAR:          equ     $FA1F  ; $FA1F: Port F Pin Assignment
SYPCR:          equ     $FA21  ; $FA21: System Protection Control, write-once
PICR:           equ     $FA22  ; $FA22: Periodic Interrupt Control
PITR:           equ     $FA24  ; $FA24: Periodic Interrupt Timer
SWSR:           equ     $FA27  ; $FA27: Software Watchdog Service
PORTC:          equ     $FA41  ; $FA41: Port C Data
CSPAR0:         equ     $FA44  ; $FA44: Chip-Select Pin Assignment 0
CSPAR1:         equ     $FA46  ; $FA46: Chip-Select Pin Assignment 1
CSBARBT:        equ     $FA48  ; $FA48: Chip-Select Base Address Boot
CSORBT:         equ     $FA4A  ; $FA4A: Chip-Select Option Boot
CSBAR0:         equ     $FA4C  ; $FA4C: Chip-Select Base Address 0; n at +4n
CSOR0:          equ     $FA4E  ; $FA4E: Chip-Select Option 0; n at +4n, up to 10

;;; Standby RAM (Table D-19)
RAMMCR:         equ     $FB00  ; $FB00: RAM Module Configuration
RAMTST:         equ     $FB02  ; $FB02: RAM Test
RAMBAH:         equ     $FB04  ; $FB04: RAM Array Base Address High
RAMBAL:         equ     $FB06  ; $FB06: RAM Array Base Address Low
