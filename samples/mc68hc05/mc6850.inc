;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; MC6850
;;; Asynchronous Communication Interface Adapter

;;; Control register
ACIA_control:   equ     ACIA+0
        ;; Counter Divider Select Bits
CDS_gm:         equ     %11    ; Group mask
CDS_DIV1_gc:    equ     %00000000 ; /1
CDS_DIV16_gc:   equ     %00000001 ; /16
CDS_DIV64_gc:   equ     %00000010 ; /64
CDS_RESET_gc:   equ     %00000011 ; Master Reset
        ;; Word Select Bits
WSB_gm:         equ     %00011100 ; Group mask
WSB_7E2_gc:     equ     %00000000 ; 7 Bits + Even Parity + 2 Stop Bits
WSB_7O2_gc:     equ     %00000100 ; 7 bits + Odd Parity  + 2 Stop Bits
WSB_7E1_gc:     equ     %00001000 ; 7 bits + Even Parity + 1 Stop Bits
WSB_7O1_gc:     equ     %00001100 ; 7 bits + Odd Parity  + 1 Stop Bits
WSB_8N2_gc:     equ     %00010000 ; 8 bits + No Parity   + 2 Stop Bits
WSB_8N1_gc:     equ     %00010100 ; 8 bits + No Parity   + 1 Stop Bits
WSB_8E1_gc:     equ     %00011000 ; 8 bits + Even Parity + 1 Stop Bits
WSB_8O1_gc:     equ     %00011100 ; 8 bits + Odd Parity  + 1 Stop Bits
        ;; Transmit Control Bits
TCB_gm:         equ     %01100000 ; Group mask
TCB_DI_gc:      equ     %00000000 ; RTS=Low,  Tx Interrupt Disabled
TCB_EI_gc:      equ     %00100000 ; RTS=Low,  Tx Interrupt Enabled
TCB_RTS_gc:     equ     %01000000 ; RTS=High, Tx Interrupt Disabled
TCB_BREAK_gc:   equ     %01100000 ; RTS=Low,  Tx Interrupt Disabled
                                  ; Transmit Break Level
RIEB_bm:        equ     %10000000 ; Receive Interrupt Enable Bit mask

;;; Status register
ACIA_status:    equ     ACIA+0
RDRF_bm:        equ     %00000001 ; Receive Data Register Full
TDRE_bm:        equ     %00000010 ; Transmit Data Register Empty
DCDF_bm:        equ     %00000100 ; Data Carrier Detect Flag
CTSF_bm:        equ     %00001000 ; Clear To Send Flag
FERR_bm:        equ     %00010000 ; Frame Error Flag
OVRN_bm:        equ     %00100000 ; Receiver Overrun Flag
PERR_bm:        equ     %01000000 ; Parity Error Flag
IRQF_bm:        equ     %10000000 ; Interrupt Request Flag

;;; Data register
ACIA_data:      equ     ACIA+1          ; Data register
