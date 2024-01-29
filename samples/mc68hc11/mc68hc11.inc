;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MC68HC11
;;; Condition Code Register (CC)
CC_CARRY      equ  %00000001   ; set to 1 if carry occurred
CC_OVERFLOW   equ  %00000010   ; set to 1 if overflow occurred
CC_ZERO       equ  %00000100   ; set to 1 if result is zero
CC_NEGATIVE   equ  %00001000   ; set to 1 if result is negative
CC_IRQ        equ  %00010000   ; if 1, IRQ is masked
CC_HALF_CARRY equ  %00100000   ; if 1, decimal carry from least digit occurred
CC_XIRQ       equ  %01000000   ; if 1, XIRQ is masked
CC_STOP       equ  %10000000   ; if 1, STOP instruction is disabled

;;; Vector
VEC_IRQ:        equ     $FFF2  ; $FFF2: #IRQ 
VEC_XIRQ:       equ     $FFF4  ; $FFF4: #XIRQ
VEC_SWI:        equ     $FFF6  ; $FFF6: Software Interrupt
VEC_ILOP:       equ     $FFF8  ; $FFF8: Illegal Opecode Trap
VEC_COP:        equ     $FFFA  ; $FFFA: COP (Watchdog) Failure
VEC_CMF:        equ     $FFFC  ; $FFFC: Clock Monitor Fail
VEC_RESET:      equ     $FFFE  ; $FFFE: Reset
