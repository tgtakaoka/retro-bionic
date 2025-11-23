;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC6801/MC6803
        cpu     6801
;;; Condition Code Register (CC)
CC_CARRY      equ  %00000001   ; set to 1 if carry occurred
CC_OVERFLOW   equ  %00000010   ; set to 1 if overflow occurred
CC_ZERO       equ  %00000100   ; set to 1 if result is zero
CC_NEGATIVE   equ  %00001000   ; set to 1 if result is negative
CC_IRQ        equ  %00010000   ; if 1, IRQ is masked
CC_HALF_CARRY equ  %00100000   ; if 1, decimal carry from least digit occurred

;;; Internal Register Area
DDR1:   equ     $00          ; $00: Port 1 Data Direction Register
DDR2:   equ     $01          ; $01: Port 2 Data Direction Register
PORT1:  equ     $02          ; $02: Port 1 Data Register
PORT2:  equ     $03          ; $03: Port 2 Data Register
DDR3:   equ     $04          ; $04: Port 3 Data Direction Register
DDR4:   equ     $05          ; $05: Port 4 Data Direction Register
PORT3:  equ     $06          ; $06: Port 3 Data Register
PORT4:  equ     $07          ; $07: Port 4 Data Register
TCSR:   equ     $08          ; $08: Timer Control and Status Register
TCNT:   equ     $09          ; $09: Timer Counter
TOCR:   equ     $0B          ; $0B: Output Compare Register
TCAP:   equ     $0D          ; $0D: Input Capture Register
P3CSR:  equ     $0F          ; $0F: Port 3 Control and Status Register
RMCR:   equ     $10          ; $10: Rate and Mode Control Registers
TRCSR:  equ     $11          ; $11: Transmit Receive Control and Status Register
SCRDR:  equ     $12          ; $12: Receiver Data Register
SCTDR:  equ     $13          ; $13: Transmit Data Register
RAMCR:  equ     $14          ; $14: RAM Control Register

;;; Port 2 Data Register
PC_gm:  equ     %11100000       ; PC group mask
PC_gp:  equ     5               ; PC group position
PC0_bm: equ     %00100000       ; PC0
PC1_bm: equ     %01000000       ; PC1
PC2_bm: equ     %10000000       ; PC2

;;; Port 3 Control and Status Register
P3_LATCH_bm:    equ     %00001000 ; Latch Enable
P3_OSS_bm:      equ     %00010000 ; Output Strobe Select
P3_IS3IE_bm:    equ     %01000000 ; IS3 IRQ1 Enable
P3_IS3F_bm:     equ     %10000000 ; IS3 Flag

;;; RAM Control Register
RAME_bm:        equ     %01000000 ; RAM Enable
STBY_PWR_bm:    equ     %10000000 ; Standby Power status

;;; Timer Control and Status Register
TCSR_OLVL_bm:   equ     %00000001 ; Output Level
TCSR_IEDG_bm:   equ     %00000010 ; Input Edge. 0=negative/1=positive
TCSR_ETOI_bm:   equ     %00000100 ; Enable Timer Overflow Interrupt for IRQ2
TCSR_EOCI_bm:   equ     %00001000 ; Enable Output Compare Interrupt for IRQ2
TCSR_EICI_bm:   equ     %00010000 ; Enable Input Capture Interrupt for IRQ2
TCSR_TOF_bm:    equ     %00100000 ; Timer Overflow Flag
TCSR_OCF_bm:    equ     %01000000 ; Output Compare Flag
TCSR_ICF_bm:    equ     %10000000 ; Input Capture Flag

;;; Serial Communication Interface
;;; Rate and Mode Control Registers
        ;; Speed Select
SS_gm:          equ     %00000011 ; Group mask
SS_DIV16_gc:    equ     %00000000 ; E/16
SS_DIV128_gc:   equ     %00000001 ; E/128
SS_DIV1024_gc:  equ     %00000010 ; E/1024
SS_DIV4096_gc:  equ     %00000011 ; E/4096
        ;; Clock Control and Format Select
CCFS_gm:        equ     %00001100 ; Group mask
CCFS_BIPH_gc:   equ     %00000000 ; Bi-Phase
CCFS_NRZ_gc:    equ     %00000100 ; NRZ
CCFS_NRZO_gc:   equ     %00001000 ; NRZ P22=Clock Output
CCFS_NRZI_gc:   equ     %00001100 ; NRZ P22=Clock Input
;;; Transmit/Receive Control and Status Register
TRCSR_WU_bm:    equ     %00000001 ; Wake-up on idle line
TRCSR_TE_bm:    equ     %00000010 ; Transmit Enable; if set, P24 is output
TRCSR_TIE_bm:   equ     %00000100 ; Transmit Interrupt Enable for IRQ2
TRCSR_RE_bm:    equ     %00001000 ; Receiver Enable; if set, P23 is input
TRCSR_RIE_bm:   equ     %00010000 ; Receiver Interrupt Enable for IRQ2
TRCSR_TDRE_bm:  equ     %00100000 ; Transmit Data Register Empty
TRCSR_ORFE_bm:  equ     %01000000 ; Overrun Framing Error
TRCSR_RDRF_bm:  equ     %10000000 ; Receive Data Register Full

;;; Vector
VEC_SCI:        equ     $FFF0  ; $FFF0: SCI Interrupt (TDRE+RDRF+ORFE)
VEC_TOF:        equ     $FFF2  ; $FFF2: Timer Overflow Interrupt
VEC_OCF:        equ     $FFF4  ; $FFF4: Output Compare Interrupt
VEC_ICF:        equ     $FFF6  ; $FFF6: Input Capture Interrupt
VEC_IRQ1:       equ     $FFF8  ; $FFF8: Maskable Interrupt Request 1
VEC_SWI:        equ     $FFFA  ; $FFFA: Software Interrupt
VEC_NMI:        equ     $FFFC  ; $FFFC: Non Maskable Interrupt
VEC_RESET:      equ     $FFFE  ; $FFFE: Reset
