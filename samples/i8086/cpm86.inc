;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; CP/M-86 1.1 in one segment at the top of memory: CCP and BDOS from
;;; CPM.SYS, then the BIOS at 2500H. The reset vector at FFFF0H is
;;; F000:FFF0, in the same segment. Memory below it, past the
;;; interrupt vectors and the debugger's park at 0400H-040FH, is the
;;; TPA.
SYSSEG:         equ     0F000H
CCP:            equ     0000H
WARM_CCP:       equ     CCP+6           ; past the autoload command
BDOS:           equ     CCP+0B00H
BDOS_ENTRY:     equ     BDOS+6
BIOS:           equ     CCP+2500H
BDOS_INT:       equ     224             ; INT E0H calls BDOS
TPA_SEG:        equ     0041H
SPT:            equ     26              ; sectors a track
