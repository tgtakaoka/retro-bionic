;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Condition Code Register (CC)
CC_CARRY      equ  %00000001   ; set to 1 if carry occurred
CC_OVERFLOW   equ  %00000010   ; set to 1 if overflow occurred
CC_ZERO       equ  %00000100   ; set to 1 if result is zero
CC_NEGATIVE   equ  %00001000   ; set to 1 if result is negative
CC_IRQ        equ  %00010000   ; if 1, IRQ is masked
CC_HALF_CARRY equ  %00100000   ; if 1, decimal carry from least digit occurred
CC_FIRQ       equ  %01000000   ; if 1, FIRQ is masked
CC_ENTIRE     equ  %10000000   ; set to 1 if entire registers are pushed

;;; Vector
VEC_SWI3:       equ     $FFF2  ; $FFF2: Software Interrupt 3
VEC_SWI2:       equ     $FFF4  ; $FFF4: Software Interrupt 2
VEC_FIRQ:       equ     $FFF6  ; $FFF6: Fast Interrupt Request
VEC_IRQ:        equ     $FFF8  ; $FFF8: Interrupt Request
VEC_SWI:        equ     $FFFA  ; $FFFA: Software Interrupt
VEC_NMI:        equ     $FFFC  ; $FFFC: Non Maskable Interrupt
VEC_RESET:      equ     $FFFE  ; $FFFE: Reset
