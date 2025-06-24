;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; TMS370
        cpu     tms370
;;; Status Register
ST_C:   equ  10000000B          ; set to 1 if carry occurred
ST_N:   equ  01000000B          ; set to 1 if result is negative
ST_Z:   equ  00100000B          ; set to 1 if result is zero
ST_V:   equ  00010000B          ; set to 1 if result is overflow
ST_IE2: equ  00001000B          ; if 1, level 2 interrupt is enabled
ST_IE1: equ  00000100B          ; if 1, level 1 interrupt is enabled
;;; Vector
VEC_INT3:       equ     7FF8H   ; #INT2 interrupt
VEC_INT2:       equ     7FFAH   ; #INT2 interrupt
VEC_INT1:       equ     7FFCH   ; #INT1 interrupt
VEC_RESET:      equ     7FFEH   ; #RESET vector
;;; Trap Vector
VEC_TRAP0:      equ     7FDEH
VEC_TRAP1:      equ     7FDCH
VEC_TRAP2:      equ     7FDAH
VEC_TRAP3:      equ     7FD8H
VEC_TRAP4:      equ     7FD6H
VEC_TRAP5:      equ     7FD4H
VEC_TRAP6:      equ     7FD2H
VEC_TRAP7:      equ     7FD0H
VEC_TRAP8:      equ     7FCEH
VEC_TRAP9:      equ     7FCCH
VEC_TRAP10:     equ     7FCAH
VEC_TRAP11:     equ     7FC8H
VEC_TRAP12:     equ     7FC6H
VEC_TRAP13:     equ     7FC4H
VEC_TRAP14:     equ     7FC2H
VEC_TRAP15:     equ     7FC0H
;;; System Control and Configuration Registers
SCCR0:          equ     1010H
OSC_POWER:      equ     01000000B
SCCR1:          equ     1011H
AUTO_WAIT_DISABLE:      equ     00010000B
SCCR2:          equ     1012H
INT1:           equ     1017H
INT2:           equ     1018H
INT3:           equ     1019H
INT_FLAG:       equ     10000000B ; INTx Flag 1:transition detected
INT_DIR:        equ     00010000B ; INTx Direction 1:output 0:input/interrupt
INT_DATA:       equ     00001000B ; INTx Data out
INT_POLALITY:   equ     00000100B ; INTx Polarity 1:rising 0:falling
INT_PRIORITY:   equ     00000010B ; INTx Priority 1:level2 0:level1
INT_ENABLE:     equ     00000001B ; INTx Enable 1:enable 0:disable
