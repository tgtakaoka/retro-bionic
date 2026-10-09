;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Z8002
        cpu     z8002

;;; Reset reads the FCW at 0002H and the PC at 0004H, the reset entry of
;;; a Program Status Area at 0000H.
ORG_RESET:      equ     0000H
;;; Program Status Area offsets: an FCW, then a PC.
PSA_EPA:        equ     04H     ; extended instruction trap
PSA_PRIV:       equ     08H     ; privileged instruction trap
PSA_SC:         equ     0CH     ; system call
PSA_NMI:        equ     14H     ; non-maskable interrupt
PSA_NVI:        equ     18H     ; non-vectored interrupt
PSA_VI:         equ     1CH     ; vectored interrupt: the FCW, then a PC
                                ; for each identifier from 1EH, the one
                                ; for identifier n at PSA_VI+2+2*n
;;; Flag and Control Word
FCW_SN:         equ     4000H   ; system mode
FCW_EPA:        equ     2000H   ; extended processor architecture
FCW_VIE:        equ     1000H   ; vectored interrupt enable
FCW_NVIE:       equ     0800H   ; non-vectored interrupt enable

;;; SC #SC_EXIT hands control back to the debugger.
SC_EXIT:        equ     0FEH
