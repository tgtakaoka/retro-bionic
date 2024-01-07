;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; i8251 USART device emulator.
MODE_STOP_gp:   equ     6
MODE_STOP_gm:   equ     %(2)11000000
MODE_STOP1_gc:  equ     (1 SHL MODE_STOP_gp)
MODE_STOP15_gc: equ     (2 SHL MODE_STOP_gp)
MODE_STOP2_gc:  equ     (3 SHL MODE_STOP_gp)
MODE_EVEN_bm:   equ     %(2)00100000
MODE_PARITY_bm: equ     %(2)00010000
MODE_LEN_gp:    equ     2
MODE_LEN_gm:    equ     %(2)00001100
MODE_LEN5_gc:   equ     (0 SHL MODE_LEN_gp)
MODE_LEN6_gc:   equ     (1 SHL MODE_LEN_gp)
MODE_LEN7_gc:   equ     (2 SHL MODE_LEN_gp)
MODE_LEN8_gc:   equ     (3 SHL MODE_LEN_gp)
MODE_BAUD_gp:   equ     0
MODE_BAUD_gm:   equ     %(2)00000011
MODE_BAUD_X1:   equ     (1 SHL MODE_BAUD_gp)
MODE_BAUD_X16:  equ     (2 SHL MODE_BAUD_gp)
MODE_BAUD_X64:  equ     (3 SHL MODE_BAUD_gp)
;;; Bit Definition of command register
CMD_EH_bm:      equ     %(2)10000000   ; Enter hunt mode
CMD_IR_bm:      equ     %(2)01000000   ; Internal Reset
CMD_RTS_bm:     equ     %(2)00100000   ; Request To Send
CMD_ER_bm:      equ     %(2)00010000   ; Error Reset
CMD_SBRK_bm:    equ     %(2)00001000   ; Send Break
CMD_RxEN_bm:    equ     %(2)00000100   ; Receive Enable
CMD_DTR_bm:     equ     %(2)00000010   ; Data Terminal Ready
CMD_TxEN_bm:    equ     %(2)00000001   ; Transmit enable
;;; Bit definition of status register
ST_DSR_bm:      equ     %(2)10000000   ; Data Set Ready
ST_BRK_bm:      equ     %(2)01000000   ; BREAK detected
ST_FE_bm:       equ     %(2)00100000   ; Framing Error
ST_OE_bm:       equ     %(2)00010000   ; Iverrun Error
ST_PE_bm:       equ     %(2)00001000   ; Parity Error
ST_TxEMPTY_bm:  equ     %(2)00000100   ; Transmitter empty
ST_RxRDY_bm:    equ     %(2)00000010   ; Receiver ready
ST_TxRDY_bm:    equ     %(2)00000001   ; Transmitter ready
;;; Interrupt name for receive/transmit interrupt
INTR_NONE:      equ     0
INTR_IRQ0:      equ     1
INTR_IRQ1:      equ     2
INTR_IRQ2:      equ     3
