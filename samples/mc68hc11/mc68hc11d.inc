;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "mc68hc11.inc"
;;; MC68HC11D
;;; Internal Register Offset
PORTA:  equ     $00             ; $00: Port A Data Register
PIOC:   equ     $02             ; $02: Parallel I/O Control
PORTC:  equ     $03             ; $03: Port C Data Register
PORTB:  equ     $04             ; $04: Port B Data Register
DDRB:   equ     $06             ; $06: Port B Data Direction Register
DDRC:   equ     $07             ; $07: Port C Data Direction Register
PORTD:  equ     $08             ; $08: Port D Data Register
DDRD:   equ     $09             ; $09: Port D Data Direction Register
CFORC:  equ     $0B             ; $0B: Timer Compare Force
OC1M:   equ     $0C             ; $0C: Output Compare 1 Mask
OC1D:   equ     $0D             ; $0D: Output Compare 1 Data
TCNT:   equ     $0E             ; $0E-$0F: Timer Counter
TIC1:   equ     $10             ; $10-$11: Timer Input Capture 1
TIC2:   equ     $12             ; $12-$13: Timer Input Capture 2
TIC3:   equ     $14             ; $14-$15: Timer Input Capture 3
TOC1:   equ     $16             ; $16-$17: Timer Output Compare 1
TOC2:   equ     $18             ; $18-$19: Timer Output Compare 2
TOC3:   equ     $1A             ; $1A-$1B: Timer Output Compare 3
TOC4:   equ     $1C             ; $1C-$1D: Timer Output Compare 4
TI4O5:  equ     $1E             ; $1E-$1F: Timer Input Capture 4/Timer Output Compare 5
TCTL1:  equ     $20             ; $20: Timer Control 1
TCTL2:  equ     $21             ; $21: Timer Control 2
TMSK1:  equ     $22             ; $22: Timer Interrupt Mask 1
TFLG1:  equ     $23             ; $23: Timer Interrupt Flag 1
TMSK2:  equ     $24             ; $24: Timer Interrupt Mask 2
TFLG2:  equ     $25             ; $25: Timer Interrupt Flag 2
PACTL:  equ     $26             ; $26: Pulse Accumulator Control
PACNT:  equ     $27             ; $27: Pulse Accumulator Count
SPCR:   equ     $28             ; $28: Serial Peripheral Control Register
SPSR:   equ     $29             ; $29: Serial Peripheral Status Register
SPDR:   equ     $2A             ; $2A: SPI Data Register 
BAUD:   equ     $2B             ; $2B: Baud Rate
SCCR1:  equ     $2C             ; $2C: SCI Control Register 1
SCCR2:  equ     $2D             ; $2D: SCI Control Register 2
SCSR:   equ     $2E             ; $2E: SCI Status Register
SCDR:   equ     $2F             ; $2F: SCI Data Register
OPTION: equ     $39             ; $39: System Configuration Options
COPRST: equ     $3A             ; $3A: Arm/Reset COP Timer Circuitry
HPRIO:  equ     $3C             ; $3C: Highest Priority I-Bit Interrupt and Miscellaneous
INIT:   equ     $3D             ; $3D: RAM and I/O Mapping Register
TEST1:  equ     $3E
CONFIG: equ     $3F             ; $3F: System Configuration

;;; PIOC: Parallel I/O Control
PIOC_CWOM_bm:   equ     %00100000 ; Port C Wired-Or Mode (1=open-drain)

;;; CFORC: Timer Compare Force
CFORC_FOC1_bm:  equ     %10000000 ; Write One to Force Compare 1
CFORC_FOC2_bm:  equ     %01000000 ; Write One to Force Compare 2
CFORC_FOC3_bm:  equ     %00100000 ; Write One to Force Compare 3
CFORC_FOC4_bm:  equ     %00010000 ; Write One to Force Compare 4
CFORC_FOC5_bm:  equ     %00001000 ; Write One to Force Compare 5

;;; OC1M: Timer Compare 1 Mask
OC1M_PA7_bm:    equ     %10000000 ; Output Compare enabled on Port A Bit 7
OC1M_PA6_bm:    equ     %01000000 ; Output Compare enabled on Port A Bit 6
OC1M_PA5_bm:    equ     %00100000 ; Output Compare enabled on Port A Bit 5
OC1M_PA4_bm:    equ     %00010000 ; Output Compare enabled on Port A Bit 4
OC1M_PA3_bm:    equ     %00001000 ; Output Compare enabled on Port A Bit 3

;;; OC1D: Timer Compare 1 Data
OC1D_PA7_bm:    equ     %10000000 ; Output Compare Data for Port A Bit 7
OC1D_PA6_bm:    equ     %01000000 ; Output Compare Data for Port A Bit 6
OC1D_PA5_bm:    equ     %00100000 ; Output Compare Data for Port A Bit 5
OC1D_PA4_bm:    equ     %00010000 ; Output Compare Data for Port A Bit 4
OC1D_PA3_bm:    equ     %00001000 ; Output Compare Data for Port A Bit 3

;;; TCTL1: Timer Control 1
TCTL1_OC2_gp:   equ     6       ; Output Compare Mode and Level for OC2
TCTL1_OC3_gp:   equ     4       ; Output Compare Mode and Level for OC3
TCTL1_OC4_gp:   equ     2       ; Output Compare Mode and Level for OC4
TCTL1_OC5_gp:   equ     0       ; Output Compare Mode and Level for OC5
TCTL1_MODE_DIS_gc:      equ     %00 ; Timer disconnected from output pin logic
TCTL1_MODE_TOGGLE_gc:   equ     %01 ; Toggle OCx output line
TCTL1_MODE_CLEAR_gc:    equ     %10 ; Clear OCx output line to 0
TCTL1_MODE_SET_gc:      equ     %11 ; Set OCx output line to 1

;;; TCTL2: Timer Control 2
TCTL2_IC4_gp:   equ     6       ; Input Capture Edge Control for IC4
TCTL2_IC1_gp:   equ     4       ; Input Capture Edge Control for IC1
TCTL2_IC2_gp:   equ     2       ; Input Capture Edge Control for IC2
TCTL2_IC3_gp:   equ     0       ; Input Capture Edge Control for IC3
TCTL2_EDG_DIS_gc:       equ     %00 ; Capture Disabled
TCTL2_EDG_RISING_gc:    equ     %01 ; Capture on rising edges only
TCTL2_EDG_FALLING_gc:   equ     %10 ; Capture on falling edges only
TCTL2_EDG_BOTH_gc:      equ     %11 ; Capture on any edge

;;; TMSK1: Timer Interrupt Mask 1
TMSK1_OC1I_bm:  equ     %10000000 ; Enable OC1 interrupt
TMSK1_OC2I_bm:  equ     %01000000 ; Enable OC2 interrupt
TMSK1_OC3I_bm:  equ     %00100000 ; Enable OC3 interrupt
TMSK1_OC4I_bm:  equ     %00010000 ; Enable OC4 interrupt
TMSK1_OC5I_bm:  equ     %00001000 ; Enable OC5 interrupt
TMSK1_IC4I_bm:  equ     %00001000 ; Enable IC4 interrupt
TMSK1_IC1I_bm:  equ     %00000100 ; Enable IC1 interrupt
TMSK1_IC2I_bm:  equ     %00000010 ; Enable IC2 interrupt
TMSK1_IC3I_bm:  equ     %00000001 ; Enable IC3 interrupt

;;; TFLG1: Timer Interrupt Flag 1
TFLG1_OC1F_bm:  equ     %10000000 ; OC1 interrupt Flag
TFLG1_OC2F_bm:  equ     %01000000 ; OC2 interrupt Flag
TFLG1_OC3F_bm:  equ     %00100000 ; OC3 interrupt Flag
TFLG1_OC4F_bm:  equ     %00010000 ; OC4 interrupt Flag
TFLG1_OC5F_bm:  equ     %00001000 ; OC5 interrupt Flag
TFLG1_IC4F_bm:  equ     %00001000 ; IC4 interrupt Flag
TFLG1_IC1F_bm:  equ     %00000100 ; IC1 interrupt Flag
TFLG1_IC2F_bm:  equ     %00000010 ; IC2 interrupt Flag
TFLG1_IC3F_bm:  equ     %00000001 ; IC3 interrupt Flag

;;; TMSK2: Timer Interrupt Mask 2
TMSK2_TOI_bm:   equ     %10000000 ; Timer Overflow Interrupt Enable
TMSK2_RTII_bm:  equ     %01000000 ; Real-time Interrupt Enable
TMSK2_PAOVI_bm: equ     %00100000 ; Pulse Accumulator Overflow Interrupt Enable
TMSK2_PAII_bm:  equ     %00010000 ; Pulse Accumulator Input Edge Interrupt Enable
TMSK2_PR_gm:    equ     %00000011 ; Timer Prescaler Select
TMSK2_PR1_gc:   equ     %00000000 ; prescaler 1
TMSK2_PR4_gc:   equ     %00000001 ; prescaler 4
TMSK2_PR8_gc:   equ     %00000010 ; prescaler 8
TMSK2_PR16_gc:  equ     %00000011 ; prescaler 16

;;; TFLG2: Timer Interrupt Flag 2
TFLG2_TOF_gm:   equ     %10000000 ; Timer Overflow Interrupt FLag
TFLG2_RTIF_gm:  equ     %01000000 ; Real-time Interrupt FLag
TFLG2_PAOVF_gm: equ     %00100000 ; Pulse Accumulator Overflow Interrupt FLag
TFLG2_PAIF_gm:  equ     %00010000 ; Pulse Accumulator Input Edge Interrupt FLag

;;; PACTL: Pulse Accumulator Control
PACTL_DDRA7_bm: equ     %10000000 ; Data Direction Control for Port A Bit 7
PACTL_PAEN_bm:  equ     %01000000 ; Pulse Accumulator System Enable
PACTL_PAMOD_gm: equ     %00110000 ; Pulse Accumulator Mode
PACTL_PAMOD_FALLING_gc: equ     %00000000 ; PAI Falling Edge Increments the Counter
PACTL_PAMOD_RISING_gc:  equ     %00010000 ; PAI Rising Edge Increments the Counter
PACTL_PAMOD_ZERO_gc:    equ     %00100000 ; A Zero on PAI Inhibits Counting
PACTL_PAMOD_ONE_gc:     equ     %00110000 ; A One on PAI Inhibits Counting
PACTL_DDRA3_bm: equ     %00001000 ; Data Direction Control for Port A Bit 3
PACTL_I4O5_bm:  equ     %00000100 ; Configure TI4/O5 Register (0=OC5, 1=IC4)
PACTL_RTR_gm:   equ     %00000011 ; Real_Time interrupt Rate
PACTL_RTR13_gc: equ     %00000000 ; E/2^13
PACTL_RTR14_gc: equ     %00000001 ; E/2^14
PACTL_RTR15_gc: equ     %00000010 ; E/2^15
PACTL_RTR16_gc: equ     %00000011 ; E/2^16

;;; SPCR: Serial Peripheral Control Register
SPCR_SPIE_bm:   equ     %10000000 ; Serial Peripheral Interrupt Enable
SPCR_SPE_bm:    equ     %01000000 ; Serial Peripheral System Enable
SPCR_DWOM_bm:   equ     %00100000 ; Port D Wired-Or Mode
SPCR_MSTR_bm:   equ     %00010000 ; Master Mode Select (0=slave, 1=master)
SPCR_CPOL_bm:   equ     %00001000 ; Clock Polarity
SPCR_CPHA_bm:   equ     %00000100 ; Clock Phase
SPCR_SPR_gm:    equ     %00000011 ; SPI Clock Rate Selects
SPCR_SPR2_gc:   equ     %00000000 ; E clock divided by 2
SPCR_SPR4_gc:   equ     %00000001 ; E clock divided by 4
SPCR_SPR16_gc:  equ     %00000010 ; E clock divided by 16
SPCR_SPR32_gc:  equ     %00000011 ; E clock divided by 32

;;; SPSR: Serial Peripheral Status Register
SPSR_SPIF_bm:   equ     %10000000 ; SPI Transfer Complete Flag
SPSR_WCOL_bm:   equ     %01000000 ; Write Collision
SPSR_MODF_bm:   equ     %00010000 ; Mode Fault

;;; BAUD: Baud Rate
BAUD_SCP_gm:    equ     %00110000 ; SCI Baud Rate Prescaler Selects
BAUD_SCP1_gc:   equ     %00000000 ; E is divided by 1
BAUD_SCP3_gc:   equ     %00010000 ; E is divided by 3
BAUD_SCP4_gc:   equ     %00100000 ; E is divided by 4
BAUD_SCP13_gc:  equ     %00110000 ; E is divided by 13
BAUD_SCR_gm:    equ     %00000111 ; SCI Baud Rate Selects (baud rate x16)
BAUD_SCR1_gc:   equ     %00000000 ; Divide Prescaler by 1
BAUD_SCR2_gc:   equ     %00000001 ; Divide Prescaler by 2
BAUD_SCR4_gc:   equ     %00000010 ; Divide Prescaler by 3
BAUD_SCR8_gc:   equ     %00000011 ; Divide Prescaler by 4
BAUD_SCR16_gc:  equ     %00000100 ; Divide Prescaler by 5
BAUD_SCR32_gc:  equ     %00000101 ; Divide Prescaler by 6
BAUD_SCR64_gc:  equ     %00000110 ; Divide Prescaler by 7
BAUD_SCR128_gc: equ     %00000111 ; Divide Prescaler by 8

;;; SCCR1: SCI Control Register 1
SCCR1_R8_bm:    equ     %10000000 ; Receive Data Bit 8
SCCR1_T8_bm:    equ     %01000000 ; Transmit Data Bit 8
SCCR1_M_bm:     equ     %00010000 ; Mode (0=8data-1stop, 1=9data-1stop)
SCCR1_WAKE_bm:  equ     %00001000 ; Wake-up by Address Mar/Idle (0=idle, 1=mark)

;;; SCCR2: SCI Control Register 2
SCCR2_TIE_bm:   equ     %10000000 ; Transmit Interrupt Enable
SCCR2_TCIE_bm:  equ     %01000000 ; Transmit Complete Interrupt Enable
SCCR2_RIE_bm:   equ     %00100000 ; Receive Interrupt Enable
SCCR2_ILIE_bm:  equ     %00010000 ; Idle Line Interrupt Enable
SCCR2_TE_bm:    equ     %00001000 ; Transmitter Enable
SCCR2_RE_bm:    equ     %00000100 ; Receiver Enable
SCCR2_RWU_bm:   equ     %00000010 ; Receiver Wake-Up Control
SCCr2_SBK_bm:   equ     %00000001 ; Send Break

;;; SCSR: SCI Status Register
SCSR_TDRE_bm:   equ     %10000000 ; Transmit Data Register Empty Flag
SCSR_TC_bm:     equ     %01000000 ; Transmit Complete Flag
SCSR_RDRF_bm:   equ     %00100000 ; Receiver Data Register Full Flag
SCSR_IDLE_bm:   equ     %00010000 ; Idle Line Detected Flag
SCSR_OR_bm:     equ     %00001000 ; Overrun Error Flag
SCSR_NF_bm:     equ     %00000100 ; Noise Error Flag
SCSR_FE_bm:     equ     %00000010 ; Framing Error

;;; OPTION: System Configuration Options
OPTION_IRQE_bm: equ     %00100000 ; #IRQ Select Edge Sensitivity only (0:level 1:edge)
OPTION_DLY_bm:  equ     %00010000 ; Enable Oscillator Startup Delay
OPTION_CME_bm:  equ     %00001000 ; Clock Monitor Enable
OPTION_CR_gm:   equ     %00000011 ; COP Timer Rate Select Bits
OPTION_CR1_gc:  equ     %00       ; Divide E/2 by 1
OPTION_CR4_gc:  equ     %01       ; Divide E/2 by 4
OPTION_CR16_gc: equ     %10       ; Divide E/2 by 16
OPTION_CR64_gc: equ     %11       ; Divide E/2 by 64

;;; COPRST: Arm/Reset COP Timer Circuitry
COPRST_ARM_gc:          equ     $55     ; Arm the COP timer clearing mechanism, then
COPRST_RESET_gc:        equ     $AA     ; Clear the COP timer.

;;; HPRIO: Highest Priority I-Bit Interrupt and Miscellaneous
HPRIO_RBOOT_bm: equ     %10000000 ; Read Bootstrap ROM
HPRIO_SMOD_bm:  equ     %01000000 ; Special Mode Select
HPRIO_MDA_bm:   equ     %00100000 ; Mode Select A
HPRIO_IRVNE_bm: equ     %00010000 ; Internal Read Visibility Enable
HPRIO_PSEL_gm:  equ     %00001111 ; Priority Select Bits
HPRIO_PSEL_TOVF_gc:     equ     %0000     ; Timer Overflow
HPRIO_PSEL_PAOVF_gc:    equ     %0001     ; Pulse Accumulator Overflow
HPRIO_PSEL_PAEDGE_gc:   equ     %0010     ; Pulse Accumulator Input Edge
HPRIO_PSEL_SPITX_gc:    equ     %0011     ; SPI Serial Transfer Complete
HPRIO_PSEL_SPI_gc:      equ     %0100     ; SPI Serial System
HPRIO_PSEL_IRQ_gc:      equ     %0110     ; #IRQ
HPRIO_PSEL_RTI_gc:      equ     %0111     ; Real Time Interrupt
HPRIO_PSEL_TIC1_gc:     equ     %1000     ; Timer Input Capture 1
HPRIO_PSEL_TIC2_gc:     equ     %1001     ; Timer Input Capture 2
HPRIO_PSEL_TIC3_gc:     equ     %1010     ; Timer Input Capture 3
HPRIO_PSEL_TIC4_gc:     equ     %1111     ; Timer Input Capture 4
HPRIO_PSEL_TOC1_gc:     equ     %1011     ; Timer Output Compare 1
HPRIO_PSEL_TOC2_gc:     equ     %1100     ; Timer Output Compare 2
HPRIO_PSEL_TOC3_gc:     equ     %1101     ; Timer Output Compare 3
HPRIO_PSEL_TOC4_gc:     equ     %1110     ; Timer Output Compare 4
HPRIO_PSEL_TOC5_gc:     equ     %1111     ; Timer Output Compare 5
        
;;; INIT: RAM and I/O Mapping Register
INIT_RAM_gm:    equ     %11110000 ; RAM Map Position
INIT_RAM_gp:    equ     4
INIT_REG_gm:    equ     %00001111 ; 54-Byte Register Block Position
INIT_REG_gp:    equ     0

;;; CONFIG: System configuration
CONFIG_NOCOP_bm:        equ     %00000100 ; COP System disable
CONFIG_ROMON_bm:        equ     %00000010 ; ROM enable

;;; Vector
VEC_SCI:        equ     $FFD6  ; $FFF0: SCI Interrupt (TDRE+TC+RDRF+IDLE+OR/NF/FE)
VEC_SPITX:      equ     $FFD8  ; $FFD8: SPI Serial Transfer Complete
VEC_PAI:        equ     $FFDA  ; $FFDA: Pulse Accumulator Input Edge
VEC_PAOV:       equ     $FFDC  ; $FFDC: Pulse Accumulator Overflow
VEC_TO:         equ     $FFDE  ; $FFDE: Timer Overflow
VEC_TOC5:       equ     $FFE0  ; $FFE0: Timer Input Compare 5 (=VEC_TICAP4)
VEC_TOC4:       equ     $FFE2  ; $FFE2: Timer Input Compare 4
VEC_TOC3:       equ     $FFE4  ; $FFE4: Timer Input Compare 3
VEC_TOC2:       equ     $FFE6  ; $FFE6: Timer Input Compare 2
VEC_TOC1:       equ     $FFE8  ; $FFE8: Timer Input Compare 1
VEC_TIC4:       equ     $FFE0  ; $FFE0: Timer Input Capture 4 (=VEC_TOCMP5)
VEC_TIC3:       equ     $FFEA  ; $FFEA: Timer Input Capture 3
VEC_TIC2:       equ     $FFEC  ; $FFEC: Timer Input Capture 2
VEC_TIC1:       equ     $FFEE  ; $FFEE: Timer Input Capture 1
VEC_RTI:        equ     $FFF0  ; $FFF0: Real Time Interrupt
