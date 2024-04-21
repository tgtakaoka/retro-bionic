;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; i8051
;;; Interrupt origin
ORG_RESET:      equ     000H    ; RESET
ORG_IE0:        equ     003H    ; #INT0
ORG_TF0:        equ     00BH    ; Timer 0 overflow
ORG_IE1:        equ     013H    ; #INT1
ORG_TF1:        equ     01BH    ; Timer 1 overflow
ORG_RITI:       equ     023H    ; Receive & Transmit
;;; Register bank and internal memory
BASE_BANK0:     equ     00H     ; Bank 0 R0-R7
BASE_BANK1:     equ     08H     ; Bank 1 R0-R7
BASE_BANK2:     equ     10H     ; Bank 2 R0-R7
BASE_BANK3:     equ     18H     ; Bank 3 R0-R7
BASE_BIT:       equ     20H     ; Bit addressable memory
BASE_MEMORY:    equ     30H     ; general purpose memory
;;; Special function registers
ACC:    equ     0E0H            ; Accumulator
B:      equ     0F0H            ; B register
PSW:    equ     0D0H            ; Program status word
SP:     equ     81H             ; Stack pointer
DPL:    equ     82H             ; DPTR low byte
DPH:    equ     83H             ; DPTR high byte
P0:     equ     80H             ; Port 0
P1:     equ     90H             ; Port 1
P2:     equ     0A0H            ; Port 2
P3:     equ     0B0H            ; Port 3
IP:     equ     0B8H            ; Interrupt priority control
IE:     equ     0A8H            ; Interrupt enable control
TMOD:   equ     89H             ; Timer/Counter mode control
TCON:   equ     88H             ; Timer/Counter control
TH0:    equ     8CH             ; Timer/Counter 0 high byte
TL0:    equ     8AH             ; Timer/Counter 0 low byte
TH1:    equ     8DH             ; Timer/Counter 1 high byte
TL1:    equ     8BH             ; Timer/Counter 1 low byte
SCON:   equ     98H             ; Serial control
SBUF:   equ     99H             ; Serial data buffer
PCON:   equ     87H             ; Power control
;;; PSW (0D0H) Program status word (bit addressable)
CY:     equ     7              ; set to 1 if carry
AC:     equ     6              ; set to 1 if auxillary carry
F0:     equ     5              ; user flag 0
RS1:    equ     4              ; register bank select
RS0:    equ     3              ; register bank select
OV:     equ     2              ; set to 1 if overflow
F1:     equ     1              ; user flag 1
P:      equ     0              ; parity bit
;;; IE (0A8H) Interrupt enable register
EA:     equ     7               ; enable interrupt
ES:     equ     4               ; Serial
ET1:    equ     3               ; Timer 1 overflow
EX1:    equ     2               ; #INT1
ET0:    equ     1               ; Timer 0 overflow
EX0:    equ     0               ; #INT0
;;; SCON (89H) Serial control (bit addressable)
SM0:    equ     7               ; Serial mode SM[0:1:2]=010 8-bit UART
SM1:    equ     6
SM2:    equ     5
REN:    equ     4               ; Enable reception
TI:     equ     1               ; Transmit interrupt flag
RI:     equ     0               ; Receive interrupt flag
;;; TCON (88H) Timer/Counter control register
TF1:    equ     7               ; Timer 1 overflow flag
TR1:    equ     6               ; Timer 1 run control
TF0:    equ     5               ; Timer 0 overflow flag
TR0:    equ     4               ; Timer 0 run control
;;; TMOD (89H) Timer/Counter mode control register
T1MODE_gp:      equ     4       ; Timer 1 mode
T0MODE_gp:      equ     0       ; Timer 0 mode
TGATE:          equ     1000B
TXPIN:          equ     0100B   ; Input from Tx pin
T8MODE:         equ     0010B   ; 8-bit auto-reload timer/counter
;;; PCON (87H) Power control register
SMOD:   equ     1000000B        ; Double baudrate
