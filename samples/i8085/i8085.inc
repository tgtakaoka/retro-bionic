;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "i8080.inc"
;;; i8085
;;; Set Innterrupt Maske
SIM_SOD:        equ     10000000B ; Serial Output Data
SIM_SDE:        equ     01000000B ; Serial Data Enable
SIM_R75:        equ     00010000B ; Reset RST 7.5 latch
SIM_MSE:        equ     00001000B ; Mask Set Enable
SIM_M75:        equ     00000100B ; RST 7.5 Mask
SIM_M65:        equ     00000010B ; RST 6.5 Mask
SIM_M55:        equ     00000001B ; RST 5.5 Mask
;;; Read Interrupt Mask
RIM_SID:        equ     10000000B ; Serial Input Data
RIM_I75:        equ     01000000B ; RST 7.5 Pending
RIM_I65:        equ     00100000B ; RST 6.5 Pending
RIM_I55:        equ     00010000B ; RST 5.5 Pending
RIM_IE:         equ     00001000B ; Interrupt Enable
RIM_M75:        equ     SIM_M75
RIM_M65:        equ     SIM_M65
RIM_M55:        equ     SIM_M55
;;; Interrupt origin
ORG_TRAP:       equ     0024H   ; TRAP
ORG_RST55:      equ     002CH   ; RST 5.5
ORG_RST65:      equ     0034H   ; RST 6.5
ORG_RST75:      equ     003CH   ; RST 7.5
