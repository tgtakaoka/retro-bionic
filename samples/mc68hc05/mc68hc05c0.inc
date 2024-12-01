;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "mc68hc05.inc"
;;; MC68HC05C0
        option  "pc-bits", 16
;;; Vector
VEC_KEY:        equ     $FFF2   ; Keyboard Scan
VEC_MFT:        equ     $FFF4   ; Multi-Function Timer
VEC_SCI:        equ     $FFF6   ; Serial Communications Interface
VEC_TIMER:      equ     $FFF8   ; 16-Bit Timer
VEC_IRQ:        equ     $FFFA   ; External Interrupt
VEC_SWI:        equ     $FFFC   ; Software
VEC_RESET:      equ     $FFFE   ; Reset

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
SCCR1:  equ     $15     ; Serial Communications Control Register 1
SCCR2:  equ     $16     ; Serial Communications Control Register 2
SCSR:   equ     $17     ; Serial Communications Status Register
SCDR:   equ     $18     ; Serial Communications Data Register
CNFGR:  equ     $19     ; Configuration Register
EICSR:  equ     $1A     ; External Interrupt Control/Status Register
        
RAM_START:      equ     $0040   ; Internal RAM $0040-$023F
STACK_TOP:      equ     $00FF   ; Stack $00FF->$0040
