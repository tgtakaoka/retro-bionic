;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; i8052
        include "i8051.inc"
;;; Interrupt origin
ORG_TF2:        equ     02BH    ; Timer 2 overflow
;;; Special function registers
T2CON:  equ     0C8H            ; Timer/Counter 2 control
TH2:    equ     0CDH            ; Timer/Counter 2 high byte
TL2:    equ     0CCH            ; Timer/Counter 2 low byte
RCAP2H: equ     0CBH            ; T/C 2 capture reg. high byte
RCAP2L: equ     0CAH            ; T/C 2 capture reg. low byte
ET2:    equ     5               ; Timer 2 overflow interrupt enable on IE (0A8H)
