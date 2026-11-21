;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; CP/M-68K 1.3's CCP and BDOS linked at 15000H with an empty BIOS at
;;; 1B000H: DRI's DISK8/CPM15000.SR for the 68000, or the 68010 system
;;; cpm68010.sub links. Both end below 1B000H, with their entry points
;;; at the same addresses, and the S-record image zeroes their BSS. The
;;; BIOS follows, and the TPA is the memory above it, up to TPA_GAP
;;; below the top of the board's address space, where the I/O devices
;;; are.
CPM:            equ     $15000          ; cold start
CCP:            equ     $150BC          ; warm start
BIOS:           equ     $1B000          ; _init
TPA:            equ     $1C000
TPA_GAP:        equ     $10000
MEM_MAX:        equ     $800000         ; Alcyon's AS68 fails with more
SPT:            equ     26              ; sectors a track
