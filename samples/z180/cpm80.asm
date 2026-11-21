;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M 2.2 cold boot on 16-bit ports: L z180/cpm80.hex loads this and
;;; mounts cpm80.dsk as A: (and cpm80b.dsk etc. as B:-D:). It reads CCP,
;;; BDOS and the BIOS from the system tracks and enters the BIOS's cold
;;; boot. For the Z180, Z280 and Z380.
        cpu     8080
        include "../i8080/cpm80.inc"

        org     0000H
        lxi     sp, 0100H
        lxi     h, CCP
        mvi     c, CCP_SECTORS+BIOS_SECTORS
        call    load_system
        jz      BIOS                    ; cold boot
        hlt
        include "../i8080/cpm80load.inc"
        include "cpm80io.inc"
