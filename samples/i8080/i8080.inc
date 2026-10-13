;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; i8080
        cpu     8080
;;; FLAGS Register
F_SIGN:         equ     10000000B ; set to 1 if negative
F_ZERO:         equ     01000000B ; set to 1 if zero
F_HALF_CARRY:   equ     00010000B ; set to 1 if carry from bit-3
F_PARITY:       equ     00000100B ; set to 1 if even parity
F_CARRY:        equ     00000001B ; set to 1 if carry
;;; Interrupt origin
ORG_RESET:      equ     0000H   ; RESET
ORG_RST0:       equ     0000H   ; RST 0
ORG_RST1:       equ     0008H   ; RST 1
ORG_RST2:       equ     0010H   ; RST 2
ORG_RST3:       equ     0018H   ; RST 3
ORG_RST4:       equ     0020H   ; RST 4
ORG_RST5:       equ     0028H   ; RST 5
ORG_RST6:       equ     0030H   ; RST 6
ORG_RST7:       equ     0038H   ; RST 3
;;; RST instructions, which the 8251 hands the CPU in an INTA cycle
INST_RST0:      equ     0C7H    ; RST 0
INST_RST1:      equ     0CFH    ; RST 1
INST_RST2:      equ     0D7H    ; RST 2
INST_RST3:      equ     0DFH    ; RST 3
INST_RST4:      equ     0E7H    ; RST 4
INST_RST5:      equ     0EFH    ; RST 5
INST_RST6:      equ     0F7H    ; RST 6
INST_RST7:      equ     0FFH    ; RST 7
