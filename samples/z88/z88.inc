;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Z88
FLAGS:  equ     %D5                  ; R/W: Flags register
F_CARRY:        equ     %(2)10000000 ; set to 1 if carry
F_ZERO:         equ     %(2)01000000 ; set to 1 if zero
F_SIGN:         equ     %(2)00100000 ; set to 1 if negative
F_OVERFLOW:     equ     %(2)00010000 ; set to 1 if overflow
F_DECIMAL_ADJ:  equ     %(2)00001000 ; decimal adjust
F_HALF_CARRY:   equ     %(2)00000100 ; set to 1 if carry from bit-3
F_FAST_INTR:    equ     %(2)00000010 ; fast interrupt status
F_BANK:         equ     %(2)00000001 ; bank address
;;;
RP0:    equ     %D6             ; R/W: Register Pointer 0
RP1:    equ     %D7             ; R/W: Register Pointer 1
SPH:    equ     %D8             ; R/W: Stack Pointer High
SPL:    equ     %D9             ; R/W: Stack Pointer Low
IPH:    equ     %DA             ; R/W: Instruction Pointer High
IPL:    equ     %DB             ; R/W: Instruction Pointer Low
;;; Interrupt
IRQ:    equ     %DC             ; R/W: Interrupt Request
IRQ_IRQ0:       equ     IMR_IRQ0
IRQ_IRQ1:       equ     IMR_IRQ1
IRQ_IRQ2:       equ     IMR_IRQ2
IRQ_IRQ3:       equ     IMR_IRQ3
IRQ_IRQ4:       equ     IMR_IRQ4
IRQ_IRQ5:       equ     IMR_IRQ5
IRQ_IRQ6:       equ     IMR_IRQ6
IRQ_IRQ7:       equ     IMR_IRQ7
IMR:    equ     %DD             ; R/W: Interrupt Mask
IMR_IRQ0:       equ     (1 SHL 0)
IMR_IRQ1:       equ     (1 SHL 1)
IMR_IRQ2:       equ     (1 SHL 2)
IMR_IRQ3:       equ     (1 SHL 3)
IMR_IRQ4:       equ     (1 SHL 4)
IMR_IRQ5:       equ     (1 SHL 5)
IMR_IRQ6:       equ     (1 SHL 6)
IMR_IRQ7:       equ     (1 SHL 7)
SYM:    equ     %DE                  ; R/W; System Mode
SYM_INTR:       equ     %(2)00000001 ; Global interrupt enable
SYM_FAST:       equ     %(2)00000010 ; Fast interrupt enable
SYM_FAST_gm:    equ     %(2)111      ; Fast interrupt select mask
SYM_FAST_gp:    equ     2            ; Fast interrupt select position
;;; Interrupt vectors
VEC_RESERVE0:   equ     %0000   ; Reserved
VEC_RESERVE1:   equ     %0002   ; Reserved
VEC_IRQ3_P32:   equ     %0004   ; IRQ3: P32 external
VEC_IRQ3_P22:   equ     %0006   ; IRQ3: P22 external
VEC_IRQ0_P23:   equ     %0008   ; IRQ0: P23 external
VEC_IRQ0_P33:   equ     %000A   ; IRQ0: P33 external
VEC_IRQ2:       equ     %000C   ; IRQ2: Counter 0 zero/P26,P27 external
VEC_IRQ5:       equ     %000E   ; IRQ5: Counter 1 zero/P36,P37 external
VEC_UART_ERROR: equ     %0010   ; IRQ6: UART overrun/framing/parity error
VEC_UART_BREAK: equ     %0012   ; IRQ6: UART break/control char/wake-up
VEC_UART_RECV:  equ     %0014   ; IRQ6: UART receive/P30 external
VEC_IRQ6_P20:   equ     %0016   ; IRQ6: P20 external
VEC_IRQ1_P21:   equ     %0018   ; IRQ1: UART zero/P21 external
VEC_UART_TRNS:  equ     %001A   ; IRQ1: UART transmit/P31 external
VEC_IRQ4:       equ     %001C   ; IRQ4: P24/P25 external
VEC_IRQ7:       equ     %001E   ; IRQ7: P34/P35 external
;;; Reset origin
ORG_RESET:      equ     %0020   ; RESET
;;; I/O Ports
PORT0:  equ     %D0                  ; Port 0
PORT1:  equ     %D1                  ; Port 1
PORT2:  equ     %D2                  ; Port 2
PORT3:  equ     %D3                  ; Port 3
PORT4:  equ     %D4                  ; Port 4
P0M:    equ     %F0                  ; W/O: Port 0 mode
P0M_ADRH:       equ     %(2)11111111  ; Port 0 is A15-A8
PM:     equ     %F1                   ; W/O: Port mode
PM_P1BUS:       equ     %(2)00100000  ; Port 1 is AD7-AD0
PM_DM:          equ     %(2)00001000  ; P35: 1=#DM, 0=I/O
H0C:    equ     %F4                   ; W/O: Handshake 0 Control
H1C:    equ     %F5                   ; W/O: Handshake 1 Control
P4D:    equ     %F6             ; W/O: Port 4 direction: 1=in, 0=out
P4OD:   equ     %F7 ; W/O: Port 4 open-drain: 1=open-drain, 0=push-pull
P2AM:   equ     %F8 ; W/O: Port 2/3 A mode; P31,P30,P21,P20
P2BM:   equ     %F9 ; W/O: Port 2/3 B mode; P33,P32,P23,P21
P2CM:   equ     %FA ; W/O: Port 2/3 C mode; P35,P34,P25,P24
P2DM:   equ     %FB ; W/O: Port 2/3 D mode; P37,P36,P27,P26
P2M_IN_gm:      equ     %(2)00     ; input
P2M_INTR_gm:    equ     %(2)01     ; input, interrupt enabled
P2M_OUTPP_gm:   equ     %(2)10     ; output, push-pull
P2M_OUTOD_gm:   equ     %(2)11     ; output, open-drain
P2AIP:  equ     %FC ; W/O: Port 2/3 A interrupt pending; P33,P32,P23,P22,P31,P30,P21,P20
P2BIP:  equ     %FD ; W/O: Port 2/3 B interrupt pending; P37,P36,P27,P26,P35,P34,P25,P24
EMT:    equ     %FE ; W/O: External Memory Timing
EMT_STACK_DM:   equ     %(2)00000010 ; Stack is on data memory
IPR:    equ     %FF             ; W/O: Interrupt Priority Register
IPR_BCA:        equ     %(2)00000010 ; B > C > A
IPR_ABC:        equ     %(2)00010000 ; A > B > C
IPR_BAC:        equ     %(2)00010010 ; B > A > C
IPR_CAB:        equ     %(2)10000000 ; C > A > B
IPR_CBA:        equ     %(2)10000010 ; C > B > A
IPR_ACB:        equ     %(2)10010000 ; A > C > B
IPR_A10:        equ     %(2)00000001 ; Group A: IRQ1 > IRQ0
IPR_B234:       equ     %(2)00000000 ; Group B: IRQ2 > IRQ3 > IRQ4
IPR_B243:       equ     %(2)00001000 ; Group B: IRQ2 > IRQ4 > IRQ3
IPR_B342:       equ     %(2)00000100 ; Group B: IRQ3 > IRQ4 > IRQ2
IPR_B432:       equ     %(2)00001100 ; Group B: IRQ4 > IRQ3 > IRQ2
IPR_C567:       equ     %(2)00000000 ; Group C: IRQ5 > IRQ6 > IRQ7
IPR_C576:       equ     %(2)01000000 ; Group C: IRQ5 > IRQ7 > IRQ6
IPR_C675:       equ     %(2)00100000 ; Group C: IRQ6 > IRQ7 > IRQ5
IPR_C765:       equ     %(2)01100000 ; Group C: IRQ7 > IRQ6 > IRQ5
;;; UART
UTC:    equ     %EB                  ; UART Transmit Control
UTC_TDMAENB:    equ     %(2)00000001 ; Transmit DMA enable
UTC_TBE:        equ     %(2)00000010 ; Transmit buffer empty
UTC_ZC:         equ     %(2)00000100 ; Zero count
UTC_TENB:       equ     %(2)00001000 ; Transmit enable
UTC_WUENB:      equ     %(2)00010000 ; Wake-up enable
UTC_STPBT2:     equ     %(2)00100000 ; 2 stop bits, 0=1 stop bit
UTC_SENBRK:     equ     %(2)01000000 ; Send break
UTC_TXDTSEL:    equ     %(2)10000000 ; Transmit data select, 1=P31 is TXD
URC:    equ     %EC                  ; UART Receive Control
URC_RCA:        equ     %(2)00000001 ; Receive character available
URC_RENB:       equ     %(2)00000010 ; Receive enable
URC_PERR:       equ     %(2)00000100 ; Parity error
URC_OVERR:      equ     %(2)00001000 ; Overrun error
URC_FERR:       equ     %(2)00010000 ; Framing error
URC_BRKD:       equ     %(2)00100000 ; Break detect
URC_CCD:        equ     %(2)01000000 ; Control Character detect
URC_WUD:        equ     %(2)10000000 ; Wake-up detect
UIE:    equ     %ED                  ; UART Interrupt Enable
UIE_RCAIE:      equ     %(2)00000001 ; Receive character available interrupt enable
UIE_RDMAENB:    equ     %(2)00000010 ; Receive DMA enable
UIE_TIE:        equ     %(2)00000100 ; Transmit interrupt enable
UIE_ZCIE:       equ     %(2)00001000 ; Zero count interrupt enable
UIE_REIE:       equ     %(2)00010000 ; Receive error interrupt enable
UIE_BRKIE:      equ     %(2)00100000 ; Break interrupt enable
UIE_CCIE:       equ     %(2)01000000 ; Control Character interrupt enable
UIE_WUIE:       equ     %(2)10000000 ; Wake-up interrupt enable
UIO:    equ     %EF                  ; UART Data
UMA:    equ     %FA                  ; BANK1: UART Mode A
UMA_EVNPAR:     equ     %(2)00000100 ; Even parity
UMA_PARENB:     equ     %(2)00001000 ; Parity enable
UMA_BCP7:       equ     %(2)00100000 ; 7 bits per Character
UMA_BCP8:       equ     %(2)00110000 ; 8 bits per Character
UMA_CR1:        equ     %(2)00000000 ; Clock rate x1
UMA_CR16:       equ     %(2)01000000 ; Clock rate x16
UMA_CR32:       equ     %(2)10000000 ; Clock rate x32
UMA_CR64:       equ     %(2)11000000 ; Clock rate x64
UMB:    equ     %FB                  ; BANK1: UART Mode B
UMB_LBENB:      equ     %(2)00000001 ; Loopback enable
UMB_BRGENB:     equ     %(2)00000010 ; Baud-rate generator enable
UMB_BRGSRC:     equ     %(2)00000100 ; Baud-rate generator source; 1=XTAL/4, 0=P20
UMB_TCIS:       equ     %(2)00001000 ; Transmit clock input select; 1=Baud-rate G., 0=P21
UMB_RCIS:       equ     %(2)00010000 ; Receive clock input select; 1=Baud-rate G., 0=P20
UMB_AE:         equ     %(2)00100000 ; Auto-echo
UMB_COS_P21:    equ     %(2)00000000 ; P21=I/O
UMB_COS_SYSTEM: equ     %(2)01000000 ; P21=System clock (XTAL/2)
UMB_COS_BAUD:   equ     %(2)10000000 ; P21=Baud-rate generator output
UMB_COS_TDR:    equ     %(2)11000000 ; P21=Transmit data rate
;;; UBRG=1/(2 x baud-rate x UMA_CR x BRG-input) - 1
UBG0:   equ     %F8             ; BANK1: UART Baud-Rate Generator 0
UBG1:   equ     %f9             ; BANK1: UART Baud-Rate Generator 1
