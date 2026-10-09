;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Z8001. A sample says `cpu z8001` for segmented code, its addresses
;;; seg<<16|off written <<seg>>off, or `cpu z8002` for code that runs
;;; nonsegmented, its addresses offsets in the PC's segment.

;;; Reset reads the FCW at 0002H, then the PC's segment at 0004H and its
;;; offset at 0006H, the reset entry of a Program Status Area at 0000H in
;;; segment 0.
ORG_RESET:      equ     0000H
;;; Program Status Area offsets: a reserved word, an FCW, then a PC's
;;; segment and offset.
PSA_EPA:        equ     08H     ; extended instruction trap
PSA_PRIV:       equ     10H     ; privileged instruction trap
PSA_SC:         equ     18H     ; system call
PSA_SEGT:       equ     20H     ; segment trap
PSA_NMI:        equ     28H     ; non-maskable interrupt
PSA_NVI:        equ     30H     ; non-vectored interrupt
PSA_VI:         equ     38H     ; vectored interrupt: the FCW, then a PC
                                ; for each even identifier from 3CH, the
                                ; one for identifier n at PSA_VI+4+2*n
;;; Flag and Control Word
FCW_SEG:        equ     8000H   ; segmented mode
FCW_SN:         equ     4000H   ; system mode
FCW_EPA:        equ     2000H   ; extended processor architecture
FCW_VIE:        equ     1000H   ; vectored interrupt enable
FCW_NVIE:       equ     0800H   ; non-vectored interrupt enable

;;; Where the segmented samples live, each in a segment of its own.
CODE:           equ     010000H ; code, segment 1
CODE_SEG:       equ     0100H   ; its segment word: the number in bits 14-8
DATA:           equ     020000H ; queues and variables, segment 2
STACK:          equ     03001000H ; RR14: segment 3, offset 1000H
;;; The normal mode stack, NSPSEG and NSPOFF: segment 4, offset 1000H.
USTACK_SEG:     equ     0400H
USTACK_OFF:     equ     1000H

;;; SC #SC_EXIT hands control back to the debugger.
SC_EXIT:        equ     0FEH
