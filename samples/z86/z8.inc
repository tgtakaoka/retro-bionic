;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Z8
FLAGS:  equ     252               ; R/W: Flags register
F_CARRY:        equ     %(2)10000000 ; set to 1 if carry
F_ZERO:         equ     %(2)01000000 ; set to 1 if zero
F_SIGN:         equ     %(2)00100000 ; set to 1 if negative
F_OVERFLOW:     equ     %(2)00010000 ; set to 1 if overflow
F_DECIMAL_ADJ:  equ     %(2)00001000 ; decimal adjust
F_HALF_CARRY:   equ     %(2)00000100 ; set to 1 if carry from bit-3
F_USER2:        equ     %(2)00000010 ; User flag F2
F_USER1:        equ     %(2)00000001 ; User flah F1
;;; Interrupt vectors
VEC_IRQ0:       equ     %0000   ; IRQ0
VEC_IRQ1:       equ     %0002   ; IRQ1
VEC_IRQ2:       equ     %0004   ; IRQ2/Tin
VEC_IRQ3:       equ     %0006   ; IRQ3/Serial in
VEC_IRQ4:       equ     %0008   ; IRQ4/Serial out/T0
VEC_IRQ5:       equ     %000A   ; IRQ5/T1
;;; Reset origin
ORG_RESET:      equ     %000C   ; RESET
;;; I/O Ports
PORT2:          equ     2       ; Port 2
PORT3:          equ     3       ; Port 3
P3M:            equ     247       ; W/O: Port 3 Mode Register
P3M_PARITY:     equ     %(2)10000000 ; 1=Parity on
P3M_SERIAL:     equ     %(2)01000000 ; 1=P30 is serial in, P37 is serial out
P3M_P2PUSHPULL: equ     %(2)00000001 ; 1=Port 2 is push-pull, 0=open drain
P2M:            equ     246       ; W/O: Port 2 Mode Register, 0=output, 1=input
P01M:           equ     248       ; W/O: Port 0 and 1 Mode Register
P01M_P0ADDR:    equ     %(2)10000010 ; Port 0 is A8~A15
P01M_P1DATA:    equ     %(2)00010000 ; Port 1 is AD0~AD7
P01M_INTERNAL:  equ     %(2)00000100 ; Stack is on internal memory
;;; Interrupt
IPR:    equ     249             ; W/O: Interrupt Priority
IPR_CAB:        equ     %(2)000001 ; C > A > B
IPR_ABC:        equ     %(2)001000 ; A > B > C
IPR_ACB:        equ     %(2)001001 ; A > C > B
IPR_BCA:        equ     %(2)010000 ; B > C > A
IPR_CBA:        equ     %(2)010001 ; C > B > A
IPR_BAC:        equ     %(2)011000 ; B > A > C
IPR_A53:        equ     %(2)000000 ; A: IRQ5 > IRQ3
IPR_A35:        equ     %(2)100000 ; A: IRQ3 > IRQ5
IPR_B20:        equ     %(2)000000 ; B: IRQ2 > IRQ0
IPR_B02:        equ     %(2)000100 ; B: IRQ0 > IRQ2
IPR_C14:        equ     %(2)000000 ; C: IRQ0 > IRQ4
IPR_C41:        equ     %(2)000010 ; C: IRQ4 > IRQ0
IMR:    equ     251             ; R/W: Interrupt Mask
IMR_ENABLE:     equ     %(2)10000000 ; Interrupt enable
IMR_IRQ0:       equ     (1 SHL 0)
IMR_IRQ1:       equ     (1 SHL 1)
IMR_IRQ2:       equ     (1 SHL 2)
IMR_IRQ3:       equ     (1 SHL 3)
IMR_IRQ4:       equ     (1 SHL 4)
IMR_IRQ5:       equ     (1 SHL 5)
IRQ:    equ     250             ; R/W: Interrupt Request
IRQ_IRQ0:       equ     IMR_IRQ0
IRQ_IRQ1:       equ     IMR_IRQ1
IRQ_IRQ2:       equ     IMR_IRQ2
IRQ_IRQ3:       equ     IMR_IRQ3
IRQ_IRQ4:       equ     IMR_IRQ4
IRQ_IRQ5:       equ     IMR_IRQ5
;;; Counter/Timers
PRE0:   equ     245             ; W/O: Prescaler 0 register
PRE0_MODULO:    equ     %(2)00000001 ; 1=Modulo-N, 0=Single-pass
PRE0_gm:        equ     %(2)11111100 ; Modulo mask
PRE0_gp:        equ     2         ; Modulo bit position
PRE1:   equ     243               ; W/O: Prescaler 1 register
PRE1_MODULO:    equ     %(2)00000001 ; 1=Modulo-N, 0=SinglePass
PRE1_INTERNAL:  equ     %(2)00000010 ; 1=T1 internal, 0=T1 external
PRE1_gm:        equ     %(2)11111100 ; Modulo mask
PRE1_gp:        equ     2         ; Modulo bit position
T1:     equ     242               ; R/W: Counter/Timer 1 Register
T0:     equ     244               ; R/W: Counter/Timer 0 Register
TMR:    equ     241               ; R/W: Timer Mode Register
TMR_LOAD_T0:    equ     %(2)00000001 ; 1=Load T0
TMR_ENABLE_T0:  equ     %(2)00000010 ; 1=Enable T0
TMR_LOAD_T1:    equ     %(2)00000100 ; 1=Load T1
TMR_ENABLE_T1:  equ     %(2)00001000 ; 1=Enable T1
TMR_TOUT_OFF:   equ     %(2)00000000 ; TOUT off
TMR_TOUT_T0:    equ     %(2)01000000 ; TOUT=T0
TMR_TOUT_T1:    equ     %(2)10000000 ; TOUT=T1
TMR_TOUT_CLOCK: equ     %(2)11000000 ; TOUT=internal clock
;;;
SIO:    equ     240             ; R/W: Serial I/O Register
RP:     equ     253             ; R/W: Register pointer
SPH:    equ     254             ; R/W: Stack Pointer High
SPL:    equ     255             ; R/W: Stack Pointer Low
