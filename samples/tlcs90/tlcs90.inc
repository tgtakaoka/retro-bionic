;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; TLCS90
;;; F Register
F_SIGN:         equ     10000000B ; set to 1 if negative
F_ZERO:         equ     01000000B ; set to 1 if zero
F_INTENB:       equ     00100000B ; set to 1 if interrupt enabled
F_HALF_CARRY:   equ     00010000B ; set to 1 if carry from bit-3
F_EXTENDED:     equ     00001000B ; set to 1 if extended carry
F_OVERFLOW:     equ     00000100B ; set to 1 if overflow
F_SUBTRACT:     equ     00000010B ; set to 1 if subtraction
F_CARRY:        equ     00000001B ; set to 1 if carry
;;; Interrupt origin
ORG_RESET:      equ     0000H   ; RESET
ORG_SWI:        equ     0010H   ; SWI
ORG_NMI:        equ     0018H   ; NMI
ORG_INTWD:      equ     0020H   ; Watchdog
ORG_INT0:       equ     0028H   ; INT0
ORG_INT1:       equ     0058H   ; INT1
