;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; CP/M 2.2 in 64K on 8" single-sided single-density disks: 77 tracks of
;;; 26 sectors of 128 bytes. The system tracks 0 and 1 hold the boot
;;; sector, then CCP and BDOS, then the BIOS.
MSIZE:  equ     64
CCP:    equ     (MSIZE-7)*1024  ; E400
BDOS:   equ     CCP+0800H       ; EC00
BIOS:   equ     CCP+1600H       ; FA00
IOBYTE: equ     0003H
TDRIVE: equ     0004H           ; current drive and user
SPT:    equ     26              ; sectors a track
CCP_SECTORS:    equ     (BIOS-CCP)/128          ; 44
BIOS_SECTORS:   equ     2*SPT-1-CCP_SECTORS     ; 7, to the end of track 1
NDISKS: equ     4

ALV_BYTES:      equ     127     ; hd8's 1016 blocks; ibm-3740 needs 31

;;; The BIOS's scratch areas, to the top of memory. The cold boot loads
;;; the BIOS's sectors over their start, which nothing has used yet.
        org     0FFFFH-128-16*NDISKS-ALV_BYTES*NDISKS   ; FD43
dirbuf: ds      128
csv:    ds      16*NDISKS
alv:    ds      ALV_BYTES*NDISKS
