;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M-8000 1.1 on the Z8001: where everything sits, shared by
;;; cpm8000bios.asm and the Makefile's link. DRI released 1.1 for the
;;; Olivetti M20, a Z8001, and this follows its memory: the system in
;;; one segment, running nonsegmented, and each program in a segment of
;;; its own.
        include "z8001.inc"

;;; The system segment, 0: the Program Status Area at 0000H, whose first
;;; entry is the reset's, the BIOS, then the kernel, CCP and BDOS as DRI
;;; linked them with its commons linked after it at SYSTEM by
;;; xout-link.py, and the system stack at the top.
PSA:            equ     0000H
;;; Through the vectored interrupts' table, identifiers 0 to 30.
PSA_SIZE:       equ     80H
BIOS_ORG:       equ     0100H
SYSTEM:         equ     2000H
SYSSTK:         equ     0FFFEH

;;; Where programs run, each segment's number in bits 14-8 of a long
;;; address's high word, as the memory region table hands them out: a
;;; merged program in 0AH; a split one's code and data both at 8, whose
;;; instruction space the debugger keeps in 0BH, the segment the loader
;;; writes the code through; DDT in 9.
SEG_MERGED:     equ     0A00H
SEG_SPLIT:      equ     0800H
SEG_ISPACE:     equ     0B00H
SEG_DDT:        equ     0900H

;;; System call numbers: SC #n.
MEM_SC:         equ     1       ; memory management and context switch
BDOS_SC:        equ     2
BIOS_SC:        equ     3
;;; The exception vector the BIOS keeps: 48 longs, traps 0-31 and system
;;; calls 0-15 as 32-47, numbered as CP/M-68K's.
NTRAPS:         equ     48
SC0TRAP:        equ     32
EPUTRAP:        equ     1
SEGTRAP:        equ     2
PITRAP:         equ     8
NMITRAP:        equ     0

;;; The trap frame the dispatcher builds on the system stack, the M20's,
;;; which DRI's kernel reads: R0-R13, the normal R14 and R15, then what
;;; the CPU pushed, the SC identifier, FCW and PC's segment and offset.
;;; A handler is called segmented, with CO bytes of return address on
;;; top, and sees each CO higher. A program's startup context, which
;;; xfer copies over the frame, has the same layout.
FR_R0:          equ     0
FR_R6:          equ     12
FR_R7:          equ     14
FR_NR14:        equ     28
FR_NR15:        equ     30
FR_ID:          equ     32
FR_FCW:         equ     34
FR_PCSEG:       equ     36
FR_PC:          equ     38
FRAME:          equ     40
CO:             equ     4

;;; The SPLIT device's control port: on, a normal-mode program fetch in
;;; segment 8 reads segment 0BH, the M20's instruction space.
SPLIT_CTRL:     equ     160H
SPLIT_ON:       equ     1
