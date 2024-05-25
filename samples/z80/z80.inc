;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; z80
;;; FLAGS Register
F_SIGN:         equ     10000000B ; set to 1 if negative
F_ZERO:         equ     01000000B ; set to 1 if zero
F_HALF_CARRY:   equ     00010000B ; set to 1 if carry from bit-3
F_PARITY:       equ     00000100B ; set to 1 if even parity
F_OVERFLOW:     equ     00000100B ; set to 1 if overflow
F_NBCD:         equ     00000010B ; set to 1 if subtraction
F_CARRY:        equ     00000001B ; set to 1 if carry
;;; Interrupt origin
ORG_RESET:      equ     0000H   ; RESET
ORG_RST00:      equ     0000H   ; RST 00H
ORG_RST08:      equ     0008H   ; RST 08H
ORG_RST10:      equ     0010H   ; RST 10H
ORG_RST18:      equ     0018H   ; RST 18H
ORG_RST20:      equ     0020H   ; RST 20H
ORG_RST28:      equ     0028H   ; RST 28H
ORG_RST30:      equ     0030H   ; RST 30H
ORG_RST38:      equ     0038H   ; RST 38H
ORG_INT:        equ     0038H   ; Mode 1 INT
ORG_NMI:        equ     0066H   ; NMI
