;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; MOS6502
;;; Processor Status Register P
P_CARRY         =       %00000001   ; set to 1 if carry occurred
P_ZERO          =       %00000010   ; set to 1 if result is zero
P_IRQ           =       %00000100   ; if 1, IRQ is disabled
P_DECIMAL       =       %00001000   ; if 1, decimal mode
P_BRK           =       %00010000   ; if 1, BRK command
P_OVERFLOW      =       %00000010   ; set to 1 if overflow occurred
P_NEGATIVE      =       %00001000   ; set to 1 if result is negative

;;; Vector
VEC_NMI         =       $FFFA  ; $FFFA: Non Maskable Interrupt
VEC_RESET       =       $FFFC  ; $FFFE: Reset
VEC_IRQ         =       $FFFE  ; $FFFE: Interrupt Request and BRK command
