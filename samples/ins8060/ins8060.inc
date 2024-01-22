;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; INS8060
;;; Status Register
S_CY    =       X'80           ; Carry bit
S_L     =       X'80           ; Link bit
S_OV    =       X'40           ; Overflow bit
S_SB    =       X'20           ; Sense B bit
S_SA    =       X'10           ; Sense A bit
S_IE    =       X'08           ; Interrupt Enable bit
S_F2    =       X'04           ; Flag 2 bit
S_F1    =       X'02           ; Flag 1 bit
S_F0    =       X'01           ; flag 0 bit

;;; Dynamic Offset by E register
OFFSET_E        =       X'80    ; offset by E register

;;; Transfer locations
ORG_RESTART     =       X'0001  ; Restart transfer location
