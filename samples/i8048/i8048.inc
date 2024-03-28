;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; i8048
;;; PSW Register
PSW_CY:         equ     10000000B ; set to 1 if carry
PSW_AC:         equ     01000000B ; set to 1 if auxillary carry
PSW_F0:         equ     00010000B ; flag 0
PSW_BS:         equ     00000100B ; register bank select
PSW_SP:         equ     00000111B ; stack pointer (grow upward)
;;; Interrupt origin
ORG_RESET:      equ     000H    ; RESET
ORG_INT:        equ     003H    ; #INT
;;; Register bank and stack
BASE_BANK0:     equ     00H     ; R0-R7
BASE_STACK:     equ     08H     ; PSW:PC
BASE_BANK1:     equ     18H     ; R0-R7
BASE_MEMORY:    equ     20H     ; general purpose memory
