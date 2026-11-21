;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Reads C sectors of drive A's system tracks into HL, from track 0
;;; sector 2, past the boot sector. Z when all were read, NZ on an error.
;;; 8080 instructions only.
load_system:
        xra     a
        call    disk_drive
        call    disk_track
        mvi     b, 2
load_sector:
        mov     a, b
        call    disk_sector
        mov     a, l
        call    disk_dmal
        mov     a, h
        call    disk_dmah
        mvi     a, CMD_READ
        call    disk_command
        call    disk_status
        ora     a
        rnz
        lxi     d, 128
        dad     d
        inr     b
        mov     a, b
        cpi     SPT+1
        jc      load_count
        mvi     b, 1                    ; on to track 1
        mvi     a, 1
        call    disk_track
load_count:
        dcr     c
        jnz     load_sector
        ret
