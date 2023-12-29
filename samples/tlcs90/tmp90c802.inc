;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; TMP90C802
        include "tlcs90.inc"
;;; Interrupt origin
ORG_INTT0:      equ     0030H   ; Timer 0
ORG_INTT1:      equ     0038H   ; Timer 1
ORG_INTT2:      equ     0040H   ; Timer 2
ORG_INTT3:      equ     0048H   ; Timer 3
ORG_INTRX:      equ     0070H   ; Serial receive
ORG_INTTX:      equ     0078H   ; Serial transmit\
;;; I/O Port
;;; Port 0,1,2 are used for data and address bus when #EA=0
;;; Port P37, P36, P35 are used for bus controls
P3:     equ     0FFC6H            ; Port 3
P32:            equ     00000100B ; Port 3.2
P3CR:   equ     0FFC7H            ; Port 3 control register
P3CR_WAIT_ENB:  equ     10000000B ; Enable #WAIT
P3CR_TXD:       equ     00001000B ; set to 1 then P33 is TXD
P3CR_RXD:       equ     00000010B ; set to 1 then P31 is RXD
P8CR:   equ     0FFD1H            ; Port 8 control register
P8CR_EDGE:      equ     00000001B ; set to 1 then INT0 is edge detection
;;; Watchdog timer
WDMOD:  equ     0FFD2H            ; Watchdog timer mode register
WDMOD_WDTE:     equ     10000000B ; set to 1 then Watchdog timer is enabled
WDCR:   equ     0FFD3H            ; Watchdog timer control register
WDCR_DISABLE:   equ     0B1H      ; Disable watchdog timer
WDCR_CLEAR:     equ     04EH      ; Clear watchdog timer
;;; Interrupt request flag
IRFL:   equ     0FFC2H
IRFL_IRF0_bp:   equ     6       ; INT0 request flag
IRFH:   equ     0FFC3H
IRFH_IRF1_bp:   equ     4       ; INT1 request flag
IRFH_IRFRX_bp:  equ     1       ; INTRX request flag
IRFH_IRFTX_bp:  equ     0       ; INTTX request flag
;;; Interrupt enable flag
INTEL:  equ     0FFE6H
INTEL_IE1_bp:   equ     4       ; enable INT1
INTEL_IERX_bp:  equ     1       ; enable INTRX
INTEL_IETX_bp:  equ     0       ; enable INTTX
INTEH:  equ     0FFE7H
INTEH_IE0_bp:   equ     2       ; enable INT0
;;; Timer/Serial channel control register
TRUN:   equ     0FFDBH
TRUN_BR9600:    equ     11000000B ; Baud rate 9600bps
TRUN_PRRUN:     equ     00100000B ; Enable prescaler
;;; Serial channel mode register
SCMOD:  equ     0FFE9H
SCMOD_RXE:      equ     00100000B ; Enable receiving data
SCMOD_SM8:      equ     00001000B ; Serial transfer mode 8-bit UART
SCMOD_SCBAUD:   equ     00000001B ; Serial transfer clock from baud rate generator
SCMOD_SCBAUD2:  equ     00000011B ; Serial transfer clock from 1/2 baud rate generator
;;; Serial channel control register
SCCR:   equ     0FFEAH
SCCR_PE_DIS:    equ     00000000B ; Disable parity
SCCR_OERR:      equ     00010000B ; Overrun error flag
SCCR_PERR:      equ     00001000B ; Parity error flag
SCCR_FERR:      equ     00000100B ; Framing error flag
;;; Serial transmission/receiving buffer register
SCBUF:  equ     0FFEBH
