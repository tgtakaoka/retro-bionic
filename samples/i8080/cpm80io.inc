;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M's port routines on 8-bit I/O addresses: the i8251 at 40H and the
;;; sector disk at 50H. Each keeps every register but A and the flags.
;;; 8080, 8085, Z80, NSC800 and KL5C80.
USART:  equ     40H
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

DISK:   equ     50H
        include "sector_disk.inc"

        cpu     8080
con_status:
        in      USARTS
        ret
con_in:
        in      USARTD
        ret
con_out:
        out     USARTD
        ret
con_control:
        out     USARTC
        ret
disk_drive:
        out     DISK_DRIVE
        ret
disk_track:
        out     DISK_TRACK
        ret
disk_sector:
        out     DISK_SECTOR
        ret
disk_dmal:
        out     DISK_DMAL
        ret
disk_dmah:
        out     DISK_DMAH
        ret
disk_command:
        out     DISK_COMMAND
        ret
disk_status:
        in      DISK_COMMAND
        ret
disk_mounted:
        in      DISK_MOUNTED
        ret
disk_format:
        in      DISK_DRIVE              ; the format's index in bits 4-7
        ret
