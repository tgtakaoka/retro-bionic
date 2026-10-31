;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; HD6413238 (H8/323) in mode 1: H8/325 Series Hardware Manual
        include "h8300.inc"
;;; Exception vectors (Table 4-2): one word each, the handler's address
VEC_RESET:      equ     $0000
VEC_HALT:       equ     $0004   ; reserved; JSR @@VEC_HALT halts to system
VEC_NMI:        equ     $0006
VEC_IRQ0:       equ     $0008
VEC_IRQ1:       equ     $000A
VEC_IRQ2:       equ     $000C
VEC_ERI0:       equ     $0024   ; SCI0 receive error
VEC_RXI0:       equ     $0026   ; SCI0 receive end
VEC_TXI0:       equ     $0028   ; SCI0 transmit end
;;; System control
SYSCR:          equ     $FFC4
SYSCR_RAME_bp:  equ     0               ; on-chip RAM enable
ISCR:           equ     $FFC6           ; IRQ sense control
IER:            equ     $FFC7           ; IRQ enable
IER_IRQ0E_bp:   equ     0
;;; Serial communication interface 0 (Table 9-2)
SMR0:           equ     $FFD8           ; serial mode
BRR0:           equ     $FFD9           ; bit rate
SCR0:           equ     $FFDA           ; serial control
TDR0:           equ     $FFDB           ; transmit data
SSR0:           equ     $FFDC           ; serial status
RDR0:           equ     $FFDD           ; receive data
SCR_TIE_bp:     equ     7               ; transmit interrupt enable
SCR_RIE_bp:     equ     6               ; receive interrupt enable
SCR_TE_bp:      equ     5               ; transmit enable
SCR_RE_bp:      equ     4               ; receive enable
SSR_TDRE_bp:    equ     7               ; transmit data register empty
SSR_RDRF_bp:    equ     6               ; receive data register full
SSR_ORER_bp:    equ     5               ; overrun error
SSR_FER_bp:     equ     4               ; framing error
SSR_PER_bp:     equ     3               ; parity error
;;; 8 bits, no parity, 1 stop bit, from ø: a bit is 32 x (BRR + 1) ø
;;; clocks, which the debugger's SCI handler counts.
SMR_8N1:        equ     0
BRR_VALUE:      equ     12

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
