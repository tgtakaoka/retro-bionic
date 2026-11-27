;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M-8000 1.1 on the Z8002: where everything sits in the 64K, shared
;;; by cpm8000bios.asm, cpm8000glue.asm and the Makefile's link.
        include "z8002.inc"

;;; The TPA runs from 0: a program is linked at 0 and the loader moves
;;; nothing, so the reset vector at 0004H belongs to the program once one
;;; has loaded. The loader also takes the TPA for a whole 64K segment: it
;;; puts the base page at 0FE00H and the program's stack below 0FD00H,
;;; whatever the memory region table says, and only sizes the program by
;;; it. So the BIOS comes first, the kernel, CCP and BDOS as DRI linked
;;; them with its commons linked after it at SYSTEM by xout-link.py, and
;;; the memory above the kernel is the program's stack.
BIOS_ORG:       equ     7400H
SYSTEM:         equ     8400H
TPA_SIZE:       equ     BIOS_ORG
;;; The Program Status Area, on a 256-byte boundary: FCW and PC pairs
;;; through the vectored interrupts' table, identifiers 0 to 15.
PSA:            equ     BIOS_ORG
PSA_SIZE:       equ     40H
;;; The system stack grows down from the kernel to the BIOS's data.
SYSSTK:         equ     SYSTEM-2

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

;;; The trap frame the dispatcher builds on the system stack, offsets
;;; from R15 in the dispatcher; a handler, called with CO bytes of return
;;; address on top, sees each CO higher. DRI's Z8002 glue (biosdefs2.z8k)
;;; shares it: R0-R14, the normal R15, then what the CPU pushed, the SC
;;; identifier, FCW and PC.
FR_R0:          equ     0
FR_R6:          equ     12
FR_R7:          equ     14
FR_R14:         equ     28
FR_NR15:        equ     30
FR_ID:          equ     32
FR_FCW:         equ     34
FR_PC:          equ     36
FRAME:          equ     38
;;; A program's startup context (the CCP's go.c), which xfer loads: R0-R15,
;;; a word it ignores, the FCW and the PC as segment and offset.
CTX_FCW:        equ     34
CTX_PC:         equ     38
CO:             equ     2

;;; The firmware's synthesized segments for a split instruction/data
;;; program: the system (the kernel and a merged program), the running
;;; program's code, and its data. A segment is the high half of the
;;; EXTMEM address the debugger forms, segment<<16 | offset.
SEG_SYS:        equ     0
SEG_CODE:       equ     1
SEG_DATA:       equ     2

;;; The SPLIT device: a control port that turns the routing on for a
;;; split program, and a copy engine that moves bytes across the
;;; segments so the BIOS, in system mode, can load a program's code and
;;; data and reach a split program's disk buffers.
SPLIT:          equ     160H
SPLIT_CTRL:     equ     SPLIT+0
SPLIT_SRCSEG:   equ     SPLIT+1
SPLIT_SRCL:     equ     SPLIT+2
SPLIT_SRCH:     equ     SPLIT+3
SPLIT_DSTSEG:   equ     SPLIT+4
SPLIT_DSTL:     equ     SPLIT+5
SPLIT_DSTH:     equ     SPLIT+6
SPLIT_COUNTL:   equ     SPLIT+7
SPLIT_COUNTH:   equ     SPLIT+8
SPLIT_ON:       equ     1
