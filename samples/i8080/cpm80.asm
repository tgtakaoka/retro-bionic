;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M 2.2 cold boot: L cpm80.hex loads this and mounts cpm80.dsk as A:
;;; (and cpm80b.dsk etc. as B:-D:). It reads CCP, BDOS and the BIOS from
;;; the system tracks and enters the BIOS's cold boot.
        include "i8080.inc"
        include "cpm80.inc"

DISK:   equ     50H
        include "sector_disk.inc"

        org     ORG_RESET
        lxi     sp, 0100H
        xra     a
        out     DISK_DRIVE
        mvi     a, 0
        out     DISK_TRACK
        mvi     b, 2                    ; sector 2: past the boot sector
        lxi     h, CCP
        mvi     c, CCP_SECTORS+BIOS_SECTORS
load:
        mov     a, b
        out     DISK_SECTOR
        mov     a, l
        out     DISK_DMAL
        mov     a, h
        out     DISK_DMAH
        mvi     a, CMD_READ
        out     DISK_COMMAND
        in      DISK_COMMAND
        ora     a
        jnz     error
        lxi     d, 128
        dad     d
        inr     b
        mov     a, b
        cpi     SPT+1
        jc      next
        mvi     b, 1                    ; on to track 1
        mvi     a, 1
        out     DISK_TRACK
next:
        dcr     c
        jnz     load
        jmp     BIOS                    ; cold boot
error:
        hlt
