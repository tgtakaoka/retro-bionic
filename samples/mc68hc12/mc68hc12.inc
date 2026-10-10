;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC68HC12 (MC68HC912BD32)
        cpu     68hc12
;;; Condition Code Register (CC)
CC_CARRY      equ  %00000001   ; set to 1 if carry occurred
CC_OVERFLOW   equ  %00000010   ; set to 1 if overflow occurred
CC_ZERO       equ  %00000100   ; set to 1 if result is zero
CC_NEGATIVE   equ  %00001000   ; set to 1 if result is negative
CC_IRQ        equ  %00010000   ; if 1, IRQ is masked
CC_HALF_CARRY equ  %00100000   ; if 1, decimal carry from least digit occurred
CC_XIRQ       equ  %01000000   ; if 1, XIRQ is masked
CC_STOP       equ  %10000000   ; if 1, STOP instruction is disabled

;;; Vector
VEC_SCI0:       equ     $FFD6  ; $FFD6: SCI0
VEC_IRQ:        equ     $FFF2  ; $FFF2: #IRQ
VEC_XIRQ:       equ     $FFF4  ; $FFF4: #XIRQ
VEC_SWI:        equ     $FFF6  ; $FFF6: Software Interrupt
VEC_TRAP:       equ     $FFF8  ; $FFF8: Unimplemented Opcode Trap
VEC_COP:        equ     $FFFA  ; $FFFA: COP (Watchdog) Failure
VEC_CMF:        equ     $FFFC  ; $FFFC: Clock Monitor Fail
VEC_RESET:      equ     $FFFE  ; $FFFE: Reset

;;; Internal memory, as reset leaves it in special expanded wide mode
REG_BASE:       equ     $0000  ; $0000-$01FF: registers
RAM_BASE:       equ     $0800  ; $0800-$0BFF: RAM
EEPROM_BASE:    equ     $0D00  ; $0D00-$0FFF: EEPROM

;;; SCI0
SC0BDH:         equ     $C0    ; Baud Rate Control High: SBR12-SBR8
SC0BDL:         equ     $C1    ; Baud Rate Control Low: SBR7-SBR0
SC0CR1:         equ     $C2    ; Control Register 1
SC0CR2:         equ     $C3    ; Control Register 2
SC0SR1:         equ     $C4    ; Status Register 1
SC0SR2:         equ     $C5    ; Status Register 2
SC0DRH:         equ     $C6    ; Data Register High: R8/T8
SC0DRL:         equ     $C7    ; Data Register Low
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
