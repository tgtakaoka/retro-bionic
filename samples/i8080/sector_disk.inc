;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; The debugger's paravirtual sector disk (debugger/sector_disk.h).
;;; DISK is its base port.
DISK_DRIVE:     equ     DISK+0  ; 0-3
DISK_TRACK:     equ     DISK+1  ; 0-76
DISK_SECTOR:    equ     DISK+2  ; 1-26, physical
DISK_DMAL:      equ     DISK+3  ; DMA address, low byte
DISK_DMAH:      equ     DISK+4  ; high byte
DISK_COMMAND:   equ     DISK+5  ; write a command, read its status
DISK_MOUNTED:   equ     DISK+6  ; bit n: drive n has its image
DISK_DMAX:      equ     DISK+7  ; DMA address bits 16-23
CMD_READ:       equ     1
CMD_WRITE:      equ     2
