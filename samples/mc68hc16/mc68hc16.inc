;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC68HC16
        cpu     68hc16
;;; Condition Code Register (CCR)
CC_CARRY      equ  %0000000100000000 ; set to 1 if carry occurred
CC_OVERFLOW   equ  %0000001000000000 ; set to 1 if overflow occurred
CC_ZERO       equ  %0000010000000000 ; set to 1 if result is zero
CC_NEGATIVE   equ  %0000100000000000 ; set to 1 if result is negative
CC_EV         equ  %0001000000000000 ; set to 1 if AM overflowed into AM[35:32]
CC_HALF_CARRY equ  %0010000000000000 ; if 1, decimal carry from least digit occurred
CC_MV         equ  %0100000000000000 ; set to 1 if AM overflowed
CC_STOP       equ  %1000000000000000 ; if 1, LPSTOP is disabled
CC_IP_gm      equ  %0000000011100000 ; interrupt priority mask; CLI and SEI
                                     ; are ANDP/ORP on it
CC_SM         equ  %0000000000010000 ; saturate mode for MAC
CC_PK_gm      equ  %0000000000001111 ; program bank

;;; Vector (CPU16RM Table 9-1); exception handlers run in bank 0
VEC_RESET:      equ     $0000   ; $0000: ZK:SK:PK, PC, SP, IZ
VEC_BKPT:       equ     $0008   ; $0008: Breakpoint
VEC_BERR:       equ     $000A   ; $000A: Bus Error
VEC_SWI:        equ     $000C   ; $000C: Software Interrupt
VEC_ILLEGAL:    equ     $000E   ; $000E: Illegal Instruction
VEC_DIV0:       equ     $0010   ; $0010: Division by Zero
VEC_UNINIT:     equ     $001E   ; $001E: Uninitialized Interrupt
VEC_IRQ1:       equ     $0022   ; $0022: Level 1 Interrupt Autovector
VEC_IRQ7:       equ     $002E   ; $002E: Level 7 Interrupt Autovector
VEC_SPURIOUS:   equ     $0030   ; $0030: Spurious Interrupt

;;; MC6850 Asynchronous Communication Interface Adapter at $FFE00:
;;; reached as ACIA_xxx,Z with ZK:IZ pointing at it.
ACIA_BASE:      equ     $FFE00
ACIA:           equ     0
