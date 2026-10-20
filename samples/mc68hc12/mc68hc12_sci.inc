;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; SCI registers, as offsets from the chip's SCI0_BASE
SC0BDH:         equ     0      ; Baud Rate Control High: SBR12-SBR8
SC0BDL:         equ     1      ; Baud Rate Control Low: SBR7-SBR0
SC0CR1:         equ     2      ; Control Register 1
SC0CR2:         equ     3      ; Control Register 2
SC0SR1:         equ     4      ; Status Register 1
SC0SR2:         equ     5      ; Status Register 2
SC0DRH:         equ     6      ; Data Register High: R8/T8
SC0DRL:         equ     7      ; Data Register Low
;;; SC0CR2
SC0CR2_TIE_bm:  equ     %10000000 ; Transmit Interrupt Enable
SC0CR2_TCIE_bm: equ     %01000000 ; Transmit Complete Interrupt Enable
SC0CR2_RIE_bm:  equ     %00100000 ; Receiver Interrupt Enable
SC0CR2_ILIE_bm: equ     %00010000 ; Idle Line Interrupt Enable
SC0CR2_TE_bm:   equ     %00001000 ; Transmitter Enable
SC0CR2_RE_bm:   equ     %00000100 ; Receiver Enable
;;; SC0SR1
SC0SR1_TDRE_bm: equ     %10000000 ; Transmit Data Register Empty
SC0SR1_TC_bm:   equ     %01000000 ; Transmit Complete
SC0SR1_RDRF_bm: equ     %00100000 ; Receive Data Register Full
SC0SR1_IDLE_bm: equ     %00010000 ; Idle Line Detected
SC0SR1_OR_bm:   equ     %00001000 ; Overrun Error
SC0SR1_NF_bm:   equ     %00000100 ; Noise Error
SC0SR1_FE_bm:   equ     %00000010 ; Framing Error
SC0SR1_PF_bm:   equ     %00000001 ; Parity Error
