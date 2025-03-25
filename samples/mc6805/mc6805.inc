;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC6805
        cpu     6805
;;; Condition Code Register (CC)
CC_CARRY      equ  %00000001   ; set to 1 if carry occurred
CC_ZERO       equ  %00000010   ; set to 1 if result is zero
CC_NEGATIVE   equ  %00000100   ; set to 1 if result is negative
CC_IRQ        equ  %00001000   ; if 1, IRQ is masked
CC_HALF_CARRY equ  %00010000   ; if 1, decimal carry from least digit occurred

;;; Internal Register Area
PORTA:  equ     $00          ; $00: Port A Data Register
PORTB:  equ     $01          ; $01: Port B Data Register
DDRA:   equ     $04          ; $04: Port A Data Direction Register
DDRB:   equ     $05          ; $05: Port B Data Direction Register
TDR:    equ     $08          ; $08: Timer Data Register
TCR:    equ     $09          ; $09: Timer Control Register

;;; Port 2 Data Register

;;; Timer Control Register
TCR_SCALER_gm:  equ     %00000111 ; Prescaler
TCR_DIV1_gc:    equ     %00000000 ; - 1/1
TCR_DIV2_gc:    equ     %00000001 ; - 1/2
TCR_DIV4_gc:    equ     %00000010 ; - 1/4
TCR_DIV8_gc:    equ     %00000011 ; - 1/8
TCR_DIV16_gc:   equ     %00000100 ; - 1/16
TCR_DIV32_gc:   equ     %00000101 ; - 1/32
TCR_DIV64_gc:   equ     %00000110 ; - 1/64
TCR_DIV128_gc:  equ     %00000111 ; - 1/128
TCR_CS_gm:      equ     %00011000 ; Timer clock source
TCR_AS_gc:      equ     %00000000 ; - Internal Clock (AS)
TCR_AS_AND_gc:  equ     %00001000 ; - Internal Clock (AS) AND TIMER input
TCR_DISABLE_gc: equ     %00010000 ; - Timer input is disabled
TCR_TIMER_gc:   equ     %00011000 ; - TIMER input
TCR_TOF_bm:     equ     %00100000 ; Timer Overflow Flag
TCR_IM_bm:      equ     %01000000 ; Timer interrupt mask
TCR_IF_bm:      equ     %10000000 ; Timer interrupt request flag
