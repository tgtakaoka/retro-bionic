;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; MC6850
;;; Asynchronous Communication Interface Adapter

;;; Control register
ACIA_control:   equ     ACIA+0
        ;; Counter Divider Select Bits
CDS_gm:         equ     11b    ; Group mask
CDS_DIV1_gc:    equ     00000000B ; /1
CDS_DIV16_gc:   equ     00000001B ; /16
CDS_DIV64_gc:   equ     00000010B ; /64
CDS_RESET_gc:   equ     00000011B ; Master Reset
        ;; Word Select Bits
WSB_gm:         equ     00011100B ; Group mask
WSB_7E2_gc:     equ     00000000B ; 7 Bits + Even Parity + 2 Stop Bits
WSB_7O2_gc:     equ     00000100B ; 7 bits + Odd Parity  + 2 Stop Bits
WSB_7E1_gc:     equ     00001000B ; 7 bits + Even Parity + 1 Stop Bits
WSB_7O1_gc:     equ     00001100B ; 7 bits + Odd Parity  + 1 Stop Bits
WSB_8N2_gc:     equ     00010000B ; 8 bits + No Parity   + 2 Stop Bits
WSB_8N1_gc:     equ     00010100B ; 8 bits + No Parity   + 1 Stop Bits
WSB_8E1_gc:     equ     00011000B ; 8 bits + Even Parity + 1 Stop Bits
WSB_8O1_gc:     equ     00011100B ; 8 bits + Odd Parity  + 1 Stop Bits
        ;; Transmit Control Bits
TCB_gm:         equ     01100000B ; Group mask
TCB_DI_gc:      equ     00000000B ; RTS=Low,  Tx Interrupt Disabled
TCB_EI_gc:      equ     00100000B ; RTS=Low,  Tx Interrupt Enabled
TCB_RTS_gc:     equ     01000000B ; RTS=High, Tx Interrupt Disabled
TCB_BREAK_gc:   equ     01100000B ; RTS=Low,  Tx Interrupt Disabled
                                  ; Transmit Break Level
RIEB_bm:        equ     10000000B ; Receive Interrupt Enable Bit mask

;;; Status register
ACIA_status:    equ     ACIA+0
RDRF_bm:        equ     00000001B ; Receive Data Register Full
TDRE_bm:        equ     00000010B ; Transmit Data Register Empty
DCDF_bm:        equ     00000100B ; Data Carrier Detect Flag
CTSF_bm:        equ     00001000B ; Clear To Send Flag
FERR_bm:        equ     00010000B ; Frame Error Flag
OVRN_bm:        equ     00100000B ; Receiver Overrun Flag
PERR_bm:        equ     01000000B ; Parity Error Flag
IRQF_bm:        equ     10000000B ; Interrupt Request Flag

;;; Data register
ACIA_data:      equ     ACIA+1          ; Data register
