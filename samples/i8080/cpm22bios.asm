;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M 2.2 BIOS: the console on the i8251 USART and four drives on the
;;; debugger's sector disk. 8080 instructions only, so it runs on the
;;; 8085 too. The image builder puts it on the system tracks after CCP
;;; and BDOS.
        include "i8080.inc"
        include "cpm22.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
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
NDISKS: equ     4

        org     BIOS
        jmp     boot
wboote: jmp     wboot
        jmp     const
        jmp     conin
        jmp     conout
        jmp     list
        jmp     punch
        jmp     reader
        jmp     home
        jmp     seldsk
        jmp     settrk
        jmp     setsec
        jmp     setdma
        jmp     read
        jmp     write
        jmp     listst
        jmp     sectran

signon:
        db      0DH, 0AH, '64K CP/M 2.2', 0DH, 0AH, 0

boot:
        lxi     sp, 0080H
        call    init_usart
        lxi     h, signon
        call    puts
        xra     a
        sta     IOBYTE
        sta     TDRIVE
        jmp     gocpm

;;; Reloads CCP and BDOS from drive A's system tracks.
wboot:
        lxi     sp, 0080H
        xra     a
        out     DISK_DRIVE
        out     DISK_TRACK
        mvi     b, 2                    ; sector 2: past the boot sector
        lxi     h, CCP
        mvi     c, CCP_SECTORS
wboot_load:
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
        jnz     wboot                   ; retry
        lxi     d, 128
        dad     d
        inr     b
        mov     a, b
        cpi     SPT+1
        jc      wboot_next
        mvi     b, 1                    ; on to track 1
        mvi     a, 1
        out     DISK_TRACK
wboot_next:
        dcr     c
        jnz     wboot_load

gocpm:
        mvi     a, 0C3H                 ; JMP
        sta     0000H
        lxi     h, wboote
        shld    0001H
        sta     0005H
        lxi     h, BDOS+6
        shld    0006H
        lxi     b, 0080H
        call    setdma
        lda     TDRIVE
        mov     c, a
        jmp     CCP

init_usart:
        xra     a
        out     USARTC
        out     USARTC
        out     USARTC                  ; safest way to sync mode
        mvi     a, CMD_IR_bm
        out     USARTC                  ; reset
        nop
        nop
        mvi     a, ASYNC_MODE
        out     USARTC
        nop
        nop
        mvi     a, RX_EN_TX_EN
        out     USARTC
        ret

puts:
        mov     a, m
        ora     a
        rz
        mov     c, a
        call    conout
        inx     h
        jmp     puts

;;; Console status: A=FFH when a character is waiting, else 0.
const:
        in      USARTS
        ani     ST_RxRDY_bm
        rz
        mvi     a, 0FFH
        ret

conin:
        call    const
        jz      conin
        in      USARTD
        ani     7FH
        ret

conout:
        in      USARTS
        ani     ST_TxRDY_bm
        jz      conout
        mov     a, c
        out     USARTD
        ret

;;; No list, punch or reader device.
list:
punch:
        ret
listst:
        xra     a
        ret
reader:
        mvi     a, 1AH                  ; end of file
        ret

home:
        mvi     c, 0
settrk:
        mov     a, c
        out     DISK_TRACK
        ret

setsec:
        mov     a, c
        out     DISK_SECTOR
        ret

setdma:
        mov     a, c
        out     DISK_DMAL
        mov     a, b
        out     DISK_DMAH
        ret

;;; HL=the drive's DPH, or 0 when it has no image.
seldsk:
        lxi     h, 0
        mov     a, c
        cpi     NDISKS
        rnc
        mov     b, a
        inr     b
        in      DISK_MOUNTED
seldsk_bit:
        rrc
        dcr     b
        jnz     seldsk_bit
        rnc
        mov     a, c
        out     DISK_DRIVE
        mov     l, c                    ; HL=C*16
        dad     h
        dad     h
        dad     h
        dad     h
        lxi     d, dph0
        dad     d
        ret

;;; A=0 when the sector moved, else 1.
read:
        mvi     a, CMD_READ
        out     DISK_COMMAND
        in      DISK_COMMAND
        ret

write:
        mvi     a, CMD_WRITE
        out     DISK_COMMAND
        in      DISK_COMMAND
        ret

;;; HL=the physical sector of logical sector BC through the table at DE.
sectran:
        xchg
        dad     b
        mov     l, m
        mvi     h, 0
        ret

;;; Skew 6, the 8" standard of DRI's skeletal CBIOS.ASM.
xlt:
        db      1, 7, 13, 19, 25, 5, 11, 17, 23, 3, 9, 15, 21
        db      2, 8, 14, 20, 26, 6, 12, 18, 24, 4, 10, 16, 22

;;; 8" single-density disk parameters: 1K blocks, 243 of them, 64
;;; directory entries, 2 system tracks.
dpb:
        dw      SPT             ; SPT, sectors a track
        db      3               ; BSH, block shift
        db      7               ; BLM, block mask
        db      0               ; EXM, extent mask
        dw      242             ; DSM, last block
        dw      63              ; DRM, last directory entry
        db      0C0H            ; AL0, the directory's blocks
        db      0               ; AL1
        dw      16              ; CKS, directory check size
        dw      2               ; OFF, system tracks

dph0:   dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*0, alv+31*0
        dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*1, alv+31*1
        dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*2, alv+31*2
        dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*3, alv+31*3

;;; Scratch areas, past the BIOS on the system tracks.
        org     BIOS+BIOS_SECTORS*128
dirbuf: ds      128
csv:    ds      16*NDISKS
alv:    ds      31*NDISKS
