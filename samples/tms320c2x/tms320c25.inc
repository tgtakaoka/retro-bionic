;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; TMS320C25
        .cpu    320c25
;;; Origin
ORG_RESET:      .equ    0000H   ; Reset
ORG_INT0:       .equ    0002H   ; #INT0
ORG_INT1:       .equ    0004H   ; #INT1
ORG_INT2:       .equ    0006H   ; #INT2
ORG_TINT:       .equ    0018H   ; Timer
ORG_RINT:       .equ    001AH   ; Serial port receive
ORG_XINT:       .equ    001CH   ; Serial port transmit
ORG_TRAP:       .equ    001EH   ; TRAP
;;; Memory mapped registers
DRR:    .equ    0               ; Serial port data receive
DXR:    .equ    1               ; Serial port data transmit
TIM:    .equ    2               ; Timer
PRD:    .equ    3               ; Period
IMR:    .equ    4               ; Interrupt mask
GREG:   .equ    5               ; Global memory allocation
;;; Interrupt mask bits
IMR_INT0:       .equ    000001B
IMR_INT1:       .equ    000010B
IMR_INT2:       .equ    000100B
IMR_TINT:       .equ    001000B
IMR_RINT:       .equ    010000B
IMR_XINT:       .equ    100000B
;;; Data memory
B2:     .equ    0060H           ; 32 words on page 0
B0:     .equ    0200H           ; 256 words on pages 4 and 5
B1:     .equ    0300H           ; 256 words on pages 6 and 7
PAGE_B0:        .equ    B0>>7
;;; Halt to system
HALT:   .equ    0CE1EH          ; TRAP
