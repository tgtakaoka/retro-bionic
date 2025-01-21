*** -*- mode: asm; mode: flyspell-prog; -*-
*** i8251 USART device emulator.
MODE_STOP_gp:   equ     6
MODE_STOP_gm:   equ     11000000B
MODE_STOP1_gc:  equ     (1 << MODE_STOP_gp)
MODE_STOP15_gc: equ     (2 << MODE_STOP_gp)
MODE_STOP2_gc:  equ     (3 << MODE_STOP_gp)
MODE_EVEN_bm:   equ     00100000B
MODE_PARITY_bm: equ     00010000B
MODE_LEN_gp:    equ     2
MODE_LEN_gm:    equ     00001100B
MODE_LEN5_gc:   equ     (0 << MODE_LEN_gp)
MODE_LEN6_gc:   equ     (1 << MODE_LEN_gp)
MODE_LEN7_gc:   equ     (2 << MODE_LEN_gp)
MODE_LEN8_gc:   equ     (3 << MODE_LEN_gp)
MODE_BAUD_gp:   equ     0
MODE_BAUD_gm:   equ     00000011B
MODE_BAUD_X1:   equ     (1 << MODE_BAUD_gp)
MODE_BAUD_X16:  equ     (2 << MODE_BAUD_gp)
MODE_BAUD_X64:  equ     (3 << MODE_BAUD_gp)
*** Bit Definition of command register
CMD_EH_bm:      equ     10000000B   Enter hunt mode
CMD_IR_bm:      equ     01000000B   Internal Reset
CMD_RTS_bm:     equ     00100000B   Request To Send
CMD_ER_bm:      equ     00010000B   Error Reset
CMD_SBRK_bm:    equ     00001000B   Send Break
CMD_RxEN_bm:    equ     00000100B   Receive Enable
CMD_DTR_bm:     equ     00000010B   Data Terminal Ready
CMD_TxEN_bm:    equ     00000001B   Transmit enable
*** Bit definition of status register
ST_DSR_bp:      equ     8       Data Set Ready
ST_BRK_bp:      equ     9       BREAK detected
ST_FE_bp:       equ     10      Framing Error
ST_OE_bp:       equ     11      Overrun Error
ST_PE_bp:       equ     12      Parity Error
ST_TxEMPTY_bp:  equ     13      Transmitter empty
ST_RxRDY_bp:    equ     14      Receiver ready
ST_TxRDY_bp:    equ     15      Transmitter ready
