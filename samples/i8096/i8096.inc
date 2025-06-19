 ;; -*- mode: asm; mode: flyspell-prog; -*-
;;; i8096
        cpu     8096
;;; SFR
ZERO:   equ     00H             ; zero register
SP:     equ     18H             ; stack pointer
INT_MASK:       equ     08H     ; interrupt mask
INT_PENDING:    equ     09H     ; interrupt pending
INT_EXTINT:        equ     1000000B ; enable EXTINT

;;; Reset origin
ORG_RESET:      equ     2080H
;;; Vector
VEC_EXTINT:     equ     200EH
VEC_TRAP:       equ     2010H
