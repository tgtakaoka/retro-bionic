;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "mc68hc05.inc"
;;; MC68HC05C0
        option  "pc-bits", 16
;;; Vector
VEC_KEY:        equ     $FFF2   ; Keyboard Scan
VEC_MFT:        equ     $FFF4   ; Multi-Function Timer
VEC_SCI:        equ     $FFF6   ; Serial Communications Interface
VEC_TIMER:      equ     $FFF8   ; 16-Bit Timer

;;; Internal Register Area
PORTB:  equ     $01     ; Port B Data Register
PORTC:  equ     $02     ; Port C Data Register
PORTD:  equ     $03     ; Port D Data Register
DDRB:   equ     $05     ; Port B Data Direction Register
DDRC:   equ     $06     ; Port C Data Direction Register
DDRD:   equ     $07     ; Port D Data Direction Register
MFTCSR: equ     $08     ; Multi-Function Timer Control/Status Register
MFTCR:  equ     $09     ; Multi-Function Timer Counter Register
TCR:    equ     $0A     ; 16-Bit Timer Control Register
TSR:    equ     $0B     ; 16-Bit Timer Status Register
ICR:    equ     $0C     ; 16-Bit Timer Input Capture Register
OCR:    equ     $0E     ; 16-Bit Timer Output Compare Register
CNTR:   equ     $10     ; 16-Bit Timer Counter Register
ACNTR:  equ     $12     ; 16-Bit Timer Alternate Counter Register
SCBR:   equ     $14     ; Serial Communications Baud Rate Register
SCBR_SCP_gp:    equ     6 ; SCI Prescaler; 0=/1 1=/3 2=/4 3=/13
SCBR_SCT_gp:    equ     3 ; SCI Transmit Baud Rate; n=1/(1<<n)
SCBR_SCR_gp:    equ     0 ; SCI Receive Baud Rate;  0<=n<=7
SCCR1:  equ     $15     ; Serial Communications Control Register 1
SCCR1_R8_bp:    equ     7 ; Receive Data Bit 8
SCCR1_T8_bp:    equ     6 ; Transmit Data Bit 8
SCCR1_SCKM_bp:  equ     5 ; Serial Clock Mode 0=-PB4 is GPIO
SCCR1_M_bp:     equ     4 ; Mode 1=9-bit, 0=8-nit
SCCR1_WAKE_bp:  equ     3 ; Wake-Up Select 1=MSB cause wake-up
SCCR1_CPOL_bp:  equ     2 ; Clock Polarity
SCCR1_CPHL_bp:  equ     1 ; Clock Phase
SCCR1_LBCL_bp:  equ     0 ; Last Bit Clock
SCCR2:  equ     $16     ; Serial Communications Control Register 2
SCCR2_TIE_bp:   equ     7 ; Transmit Interrupt Enable
SCCR2_TCIE_bp:  equ     6 ; Transmit Complete Interrupt Enable
SCCR2_RIE_bp:   equ     5 ; Receive Interrupt Enable
SCCR2_ILIE_bp:  equ     4 ; Idle Line Interrupt Enable
SCCR2_TE_bp:    equ     3 ; Transmit Enable
SCCR2_RE_bp:    equ     2 ; Receive Enable
SCCR2_RWU_bp:   equ     1 ; Receiver Wake-up
SCCR2_SBK_bp:   equ     0 ; Send Break
SCSR:   equ     $17     ; Serial Communications Status Register
SCSR_TDRE_bp    equ     7 ; Transmit Data Register Empty Flag
SCSR_TC_bp:     equ     6 ; Transmit Complete Flag
SCSR_RDRF_bp:   equ     5 ; Receive Data Register Full Flag
SCSR_IDLE_bp:   equ     4 ; Idle Line Detected Flag
SCSR_OR_bp:     equ     3 ; Overrun Error Flag
SCSR_NF_bp:     equ     2 ; Noise Error Flag
SCSR_FE_bp      equ     1 ; Framing Error Flag
SCDR:   equ     $18     ; Serial Communications Data Register
CNFGR:  equ     $19     ; Configuration Register
EICSR:  equ     $1A     ; External Interrupt Control/Status Register
EICSR_KSF_bp:   equ     7 ; Keyboard Scan Request Flag
EICSR_KSEN_bp:  equ     6 ; Keyboard Scan Enable
EICSR_KSA_bp:   equ     5 ; Keyboard Scan Acknowledge
EICSR_KSEL_bp:  equ     4 ; Keyboard Scan Edge/Level
EICSR_IRQF_bp:  equ     3 ; Interrupt Request Flag
EICSR_IRQEN_bp: equ     2 ; Interrupt Request Enable
EICSR_IRQA_bp:  equ     1 ; Interrupt Request Acknowledge
EICSR_IRQEL_bp: equ     0 ; Interrupt Request Edge/Level

RAM_START:      equ     $0040   ; Internal RAM $0040-$023F
STACK_TOP:      equ     $00FF   ; Stack $00FF->$0040
