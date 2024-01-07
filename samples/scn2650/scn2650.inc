;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; PSU
PSU_S:  equ     B'10000000'  ; Sense
PSU_F:  equ     B'01000000'  ; Flag
PSU_II: equ     B'00100000'  ; Interrupt inhibit
PSU_SP: equ     B'00000111'  ; Stack pointer
;;; PSL
PSL_CC1:        equ     B'10000000' ; Condition code 1 (10=Negative)
PSL_CC0:        equ     B'01000000' ; Condition code 0 (01=Positive)
PSL_IDC:        equ     B'00100000' ; Inter-digit carry
PSL_RS:         equ     B'00010000' ; Register bank select
PSL_WC:         equ     B'00001000' ; With/Without carry
PSL_OVF:        equ     B'00000100' ; Overflow
PSL_COM:        equ     B'00000010' ; Logical arithmetic compare
PSL_C:          equ     B'00000001' ; Carry
;;; Reset origin
ORG_RESET:      equ     H'0000'
