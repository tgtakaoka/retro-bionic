;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M 2.2 on a V20/V30 in 8080 emulation mode. The Makefile appends
;;; ../i8080/cpm80.hex, the 8080 cold boot, to this image at
;;; SEG_I8080:0000; L v30/cpm80.hex loads both and mounts cpm80.dsk
;;; beside it, whose BIOS polls the USART and runs with interrupts off.
;;; The disk's DMAX puts the BIOS's 16-bit DMA addresses in SEG_I8080.
        include "../i8080/i8080.inc"
        cpu     V30

SEG_I8080:      equ     1000H   ; CP/M's 64K
VEC_I8080:      equ     22H     ; BRKEM's vector to its reset entry
DISK:           equ     50H
        include "../i8080/sector_disk.inc"

        org     VEC_I8080*4
        dw      ORG_RESET, SEG_I8080

        org     1000H
stack:  equ     $

init:
        mov     ax, SEG_I8080
        mov     ds, ax          ; DS0, the 8080 data segment
        xor     ax, ax
        mov     ss, ax
        mov     sp, stack
        mov     al, SEG_I8080/1000H     ; DMA bits 16-23
        out     DISK_DMAX, al
        brkem   VEC_I8080
        jmp     $               ; CP/M never returns

        org     0FFFF0H
        jmpf    0:init

        end
