;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC6850 Asynchronous Communication Interface Adapter
;;; One register per word, accessed by byte.
ACIA_control    =       ACIA+0
ACIA_status     =       ACIA+0
ACIA_data       =       ACIA+2
ACIA_intr       =       ACIA+4  ; Interrupt vector, for Rx and Tx
;;;  Counter Divider Select Bits
CDS_gm          =       003     ; Group mask
CDS_DIV1_gc     =       000     ; /1
CDS_DIV16_gc    =       001     ; /16
CDS_DIV64_gc    =       002     ; /64
CDS_RESET_gc    =       003     ; Master Reset
;;;  Word Select Bits
WSB_gm          =       034     ; Group mask
WSB_7E2_gc      =       000     ; 7 Bits + Even Parity + 2 Stop Bits
WSB_7O2_gc      =       004     ; 7 bits + Odd Parity  + 2 Stop Bits
WSB_7E1_gc      =       010     ; 7 bits + Even Parity + 1 Stop Bits
WSB_7O1_gc      =       014     ; 7 bits + Odd Parity  + 1 Stop Bits
WSB_8N2_gc      =       020     ; 8 bits + No Parity   + 2 Stop Bits
WSB_8N1_gc      =       024     ; 8 bits + No Parity   + 1 Stop Bits
WSB_8E1_gc      =       030     ; 8 bits + Even Parity + 1 Stop Bits
WSB_8O1_gc      =       034     ; 8 bits + Odd Parity  + 1 Stop Bits
;;;  Transmit Control Bits
TCB_gm          =       140     ; Group mask
TCB_DI_gc       =       000     ; RTS=Low,  Tx Interrupt Disabled
TCB_EI_gc       =       040     ; RTS=Low,  Tx Interrupt Enabled
TCB_RTS_gc      =       100     ; RTS=High, Tx Interrupt Disabled
TCB_BREAK_gc    =       140     ; RTS=Low,  Tx Interrupt Disabled
                                ; Transmit Break Level
RIEB_bm         =       200     ; Receive Interrupt Enable Bit mask
;;; Status register
RDRF_bm         =       001     ; Receive Data Register Full
TDRE_bm         =       002     ; Transmit Data Register Empty
DCDF_bm         =       004     ; Data Carrier Detect Flag
CTSF_bm         =       010     ; Clear To Send Flag
FERR_bm         =       020     ; Frame Error Flag
OVRN_bm         =       040     ; Receiver Overrun Flag
PERR_bm         =       100     ; Parity Error Flag
IRQF_bm         =       200     ; Interrupt Request Flag
