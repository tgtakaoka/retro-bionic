;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC68000 family
        cpu     mc68000

;;; Status Register (SR)
SR_T:           equ     %1000000000000000 ; trace
SR_S:           equ     %0010000000000000 ; supervisor
SR_I_gm:        equ     %0000011100000000 ; interrupt priority mask
SR_X:           equ     %0000000000010000 ; extend
SR_N:           equ     %0000000000001000 ; negative
SR_Z:           equ     %0000000000000100 ; zero
SR_V:           equ     %0000000000000010 ; overflow
SR_C:           equ     %0000000000000001 ; carry

;;; Exception vectors with VBR 0 (M68000UM Table 6-2)
VEC_RESET:      equ     $000000 ; $000000: SSP, then PC
VEC_BERR:       equ     $000008 ; $000008: Bus Error
VEC_ADDR:       equ     $00000C ; $00000C: Address Error
VEC_ILLEGAL:    equ     $000010 ; $000010: Illegal Instruction
VEC_DIV0:       equ     $000014 ; $000014: Zero Divide
VEC_CHK:        equ     $000018 ; $000018: CHK Instruction
VEC_TRAPV:      equ     $00001C ; $00001C: TRAPV Instruction
VEC_PRIV:       equ     $000020 ; $000020: Privilege Violation
VEC_TRACE:      equ     $000024 ; $000024: Trace
VEC_UNINIT:     equ     $00003C ; $00003C: Uninitialized Interrupt
VEC_SPURIOUS:   equ     $000060 ; $000060: Spurious Interrupt
VEC_LEVEL1:     equ     $000064 ; $000064: Level 1 Interrupt Autovector
VEC_LEVEL7:     equ     $00007C ; $00007C: Level 7 Interrupt Autovector
VEC_TRAP0:      equ     $000080 ; $000080: TRAP #0
VEC_USER:       equ     $000100 ; $000100: User Interrupt Vectors, 64-255

;;; Every interrupt is acknowledged with a vector number: the ACIA's is
;;; the first user vector, as the regress wires it (I ACIA addr 64).
VECNUM_ACIA:    equ     64
VEC_ACIA:       equ     VECNUM_ACIA*4

;;; MC6850 Asynchronous Communication Interface Adapter at $FFFFF0,
;;; reached as (ACIA_xxx).w on every board: a board with fewer address
;;; lines sees it masked to them.
ACIA:           equ     $FFFFF0
        include "../mc6800/mc6850.inc"

;;; Interrupts on, all levels; and off again. Neither touches CCR.
INTR_ON:        equ     $F8FF   ; ANDI #INTR_ON,SR
INTR_OFF:       equ     $0700   ; ORI  #INTR_OFF,SR
