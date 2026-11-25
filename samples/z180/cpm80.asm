;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M 2.2 cold boot on 16-bit ports: L z180/cpm80.hex loads this and
;;; mounts cpm80a.dsk as A: (and cpm80b.dsk etc. as B:-D:). It reads CCP,
;;; BDOS and the BIOS from the system tracks and enters the BIOS's cold
;;; boot. For the Z180, Z280 and Z380.
;;;
;;; The Z380's memory decodes all its address lines until told otherwise,
;;; and CP/M's programs count on 64K wrapping, so the boot sets its
;;; address mask register to 16 bits. The port is nothing on the others.
        cpu     8080
        include "../i8080/cpm80.inc"

        org     0000H
        lxi     sp, 0100H
        mvi     a, 16
        call    addr_mask
        lxi     h, CCP
        mvi     c, CCP_SECTORS+BIOS_SECTORS
        call    load_system
        jz      BIOS                    ; cold boot
        hlt
        include "../i8080/cpm80load.inc"
        include "cpm80io.inc"

ADDR_MASK:      equ     0148H   ; address bits memory decodes
addr_mask:
        push    BC
        ld      BC, ADDR_MASK
        jr      port_out
