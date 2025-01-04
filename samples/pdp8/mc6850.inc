/// -*- mode: asm; mode: flyspell-prog; -*-
/// MC6850
/// Asynchronous Communication Interface Adapter
ACIA_status     = 0
ACIA_control    = 4
ACIA_receive    = 1
ACIA_transmit   = 5
//  Counter Divider Select Bits
CDS_gm          =       011B    / Group mask
CDS_DIV1_gc     =       000B    / /1
CDS_DIV16_gc    =       001B    / /16
CDS_DIV64_gc    =       010B    / /64
CDS_RESET_gc    =       011B    / Master Reset
//  Word Select Bits
WSB_gm          =       011100B / Group mask
WSB_7E2_gc      =       000000B / 7 Bits + Even Parity + 2 Stop Bits
WSB_7O2_gc      =       000100B / 7 bits + Odd Parity  + 2 Stop Bits
WSB_7E1_gc      =       001000B / 7 bits + Even Parity + 1 Stop Bits
WSB_7O1_gc      =       001100B / 7 bits + Odd Parity  + 1 Stop Bits
WSB_8N2_gc      =       010000B / 8 bits + No Parity   + 2 Stop Bits
WSB_8N1_gc      =       010100B / 8 bits + No Parity   + 1 Stop Bits
WSB_8E1_gc      =       011000B / 8 bits + Even Parity + 1 Stop Bits
WSB_8O1_gc      =       011100B / 8 bits + Odd Parity  + 1 Stop Bits
//  Transmit Control Bits
TCB_gm          =     01100000B / Group mask
TCB_DI_gc       =     00000000B / RTS=Low,  Tx Interrupt Disabled
TCB_EI_gc       =     00100000B / RTS=Low,  Tx Interrupt Enabled
TCB_RTS_gc      =     01000000B / RTS=High, Tx Interrupt Disabled
TCB_BREAK_gc    =     01100000B / RTS=Low,  Tx Interrupt Disabled
                                / Transmit Break Level
RIEB_bm         =     10000000B / Receive Interrupt Enable Bit mask
/// Status register
RDRF_bm         =     00000001B / Receive Data Register Full
TDRE_bm         =     00000010B / Transmit Data Register Empty
DCDF_bm         =     00000100B / Data Carrier Detect Flag
CTSF_bm         =     00001000B / Clear To Send Flag
FERR_bm         =     00010000B / Frame Error Flag
OVRN_bm         =     00100000B / Receiver Overrun Flag
PERR_bm         =     01000000B / Parity Error Flag
IRQF_bm         =     10000000B / Interrupt Request Flag
