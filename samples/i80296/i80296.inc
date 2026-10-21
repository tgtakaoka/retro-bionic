;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; i80296: the 80296SA, in 64-Kbyte mode
        include "../i8096/i8096.inc"
        cpu     80296
;;; Port 2, where EXTINT0 is P2.2's special function
P2_MODE:        equ     1FD1H
P2_DIR:         equ     1FD3H
P2_REG:         equ     1FD5H
P2_EXTINT0:     equ     00000100B
INT_EXTINT0:    equ     00001000B
