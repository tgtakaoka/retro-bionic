 ;; -*- mode: asm; mode: flyspell-prog; -*-
;;; i8096
        cpu     8096
;;; SFR
ZERO:   equ     00H             ; zero register
SP:     equ     18H             ; stack pointer
INT_MASK:       equ     08H     ; interrupt mask
INT_PENDING:    equ     09H     ; interrupt pending
;;; EXTINT, vector 200EH, on the 8X9XBH and 80C196KB/KC; EXTINT0,
;;; vector 2006H, on the 80C196NP and 80296SA
INT_EXTINT:     equ     10001000B

;;; Reset origin
ORG_RESET:      equ     2080H
;;; Vector
VEC_EXTINT0:    equ     2006H
VEC_EXTINT:     equ     200EH
VEC_TRAP:       equ     2010H
