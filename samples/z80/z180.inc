;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z80.inc"
        cpu     z180
;;; WAIT Control
DCNTL:  equ     32H                ; DMA/WAIT Control Register
DCNTL_MWI_bp:   equ     6          ; Memory Wait Insertion (bit position)
DCNTL_MWI_bm:   equ     0C0H       ; (bit mask)
DCNTL_IWI_bp:   equ     4          ; I/O Wait Insertion (bit position)
DCNTL_IWI_bm:   equ     30H        ; (bit mask)
;;; Interrupt controller
IL:     equ     33H                ; Interrupt Vector Low Register
ITC:    equ     34H                ; INT/TRAP Control Register
ITC_TRAP_bp:    equ     7          ; TRAP
ITC_UFO_bp:     equ     6          ; Undefined Fetch Object
ITC_ITE2_bp:    equ     2          ; Interrupt Enable 2
ITC_ITE1_bp:    equ     1          ; Interrupt Enable 2
ITC_ITE0_bp:    equ     0          ; Interrupt Enable 2
