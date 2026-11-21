;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M's port routines on 16-bit I/O addresses: the i8251 at 0140H and
;;; the sector disk at 0150H, clear of the on-chip I/O. Each keeps every
;;; register but A and the flags. Z180, Z280 and Z380.
        include "usart.inc"
ST_RxRDY_bm:    equ     1 << ST_RxRDY_bp
ST_TxRDY_bm:    equ     1 << ST_TxRDY_bp
DISK:   equ     0150H
        include "../i8080/sector_disk.inc"

        cpu     z80
con_status:
        push    BC
        ld      BC, USARTS
        jr      port_in
con_in:
        push    BC
        ld      BC, USARTD
        jr      port_in
con_out:
        push    BC
        ld      BC, USARTD
        jr      port_out
con_control:
        push    BC
        ld      BC, USARTC
        jr      port_out
disk_drive:
        push    BC
        ld      BC, DISK_DRIVE
        jr      port_out
disk_track:
        push    BC
        ld      BC, DISK_TRACK
        jr      port_out
disk_sector:
        push    BC
        ld      BC, DISK_SECTOR
        jr      port_out
disk_dmal:
        push    BC
        ld      BC, DISK_DMAL
        jr      port_out
disk_dmah:
        push    BC
        ld      BC, DISK_DMAH
        jr      port_out
disk_command:
        push    BC
        ld      BC, DISK_COMMAND
        jr      port_out
disk_status:
        push    BC
        ld      BC, DISK_COMMAND
        jr      port_in
disk_mounted:
        push    BC
        ld      BC, DISK_MOUNTED
        jr      port_in
disk_format:
        push    BC
        ld      BC, DISK_DRIVE          ; the format's index in bits 4-7
port_in:
        in      A, (C)
        pop     BC
        ret
port_out:
        out     (C), A
        pop     BC
        ret
