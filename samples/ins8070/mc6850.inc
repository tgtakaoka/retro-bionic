;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; MC6850
;;; Asynchronous Communication Interface Adapter

;;; Control register
ACIA_control    =       ACIA+0
        ;; Counter Divider Select Bits
CDS_gm          =       X'03   ; Group mask
CDS_DIV1_gc     =       X'00   ; /1
CDS_DIV16_gc    =       X'01   ; /16
CDS_DIV64_gc    =       X'02   ; /64
CDS_RESET_gc    =       X'03   ; Master Reset
        ;; Word Select Bits
WSB_gm          =       X'1C   ; Group mask
WSB_7E2_gc      =       X'00   ; 7 Bits + Even Parity + 2 Stop Bits
WSB_7O2_gc      =       X'04   ; 7 bits + Odd Parity  + 2 Stop Bits
WSB_7E1_gc      =       X'08   ; 7 bits + Even Parity + 1 Stop Bits
WSB_7O1_gc      =       X'0C   ; 7 bits + Odd Parity  + 1 Stop Bits
WSB_8N2_gc      =       X'10   ; 8 bits + No Parity   + 2 Stop Bits
WSB_8N1_gc      =       X'14   ; 8 bits + No Parity   + 1 Stop Bits
WSB_8E1_gc      =       X'18   ; 8 bits + Even Parity + 1 Stop Bits
WSB_8O1_gc      =       X'1C   ; 8 bits + Odd Parity  + 1 Stop Bits
        ;; Transmit Control Bits
TCB_gm          =       X'60   ; Group mask
TCB_DI_gc       =       X'00   ; RTS=Low,  Tx Interrupt Disabled
TCB_EI_gc       =       X'20   ; RTS=Low,  Tx Interrupt Enabled
TCB_RTS_gc      =       X'40   ; RTS=High, Tx Interrupt Disabled
TCB_BREAK_gc    =       X'60   ; RTS=Low,  Tx Interrupt Disabled
                                ; Transmit Break Level
RIEB_bm         =       X'80   ; Receive Interrupt Enable Bit mask

;;; Status register
ACIA_status     =       ACIA+0
RDRF_bm         =       X'01   ; Receive Data Register Full
TDRE_bm         =       X'02   ; Transmit Data Register Empty
DCDF_bm         =       X'04   ; Data Carrier Detect Flag
CTSF_bm         =       X'08   ; Clear To Send Flag
FERR_bm         =       X'10   ; Frame Error Flag
OVRN_bm         =       X'20   ; Receiver Overrun Flag
PERR_bm         =       X'40   ; Parity Error Flag
IRQF_bm         =       X'80   ; Interrupt Request Flag

;;; Data register
ACIA_data       =       ACIA+1  ; Data register
