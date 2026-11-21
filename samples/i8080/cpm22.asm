;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M 2.2 cold boot on 8-bit ports: L i8080/cpm22.hex loads this and
;;; mounts cpm22.dsk as A: (and cpm22b.dsk etc. as B:-D:). It reads CCP,
;;; BDOS and the BIOS from the system tracks and enters the BIOS's cold
;;; boot. For the 8080, 8085, Z80, NSC800 and KL5C80.
        include "i8080.inc"
        include "cpm22.inc"

        org     ORG_RESET
        lxi     sp, 0100H
        lxi     h, CCP
        mvi     c, CCP_SECTORS+BIOS_SECTORS
        call    load_system
        jz      BIOS                    ; cold boot
        hlt
        include "cpm22load.inc"
        include "cpm22io.inc"
