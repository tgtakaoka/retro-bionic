;;; -*- mode: asm; mode: flyspell-prog; -*-

        .include "mos6502.inc"

;;; W65C816S
;;; Processor Status Register P
P_X     =       %00010000       ; if 0, X and Y are 16 bit.
P_M     =       %00100000       ; if 0, A is 16 bit.

;;; Emulation Mode Vector Locations
VEC_ABORT       =       $FFF8   ; $FFF8: Abort
VEC_COP         =       $FFF4   ; $FFF4: COP command

;;; Native Mode Vector Locations
NVEC_IRQ        =       $FFEE   ; $FFEE: Interrupt Request
NVEC_NMI        =       $FFEA   ; $FFEA: Non Maskable Interrupt
NVEC_ABORT      =       $FFE8   ; $FFE8: Abort
NVEC_BRK        =       $FFE6   ; $FFE6: BRK command
NVEC_COP        =       $FFE4   ; $FFE4: COP command
