;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M 2.2 BIOS: the console on the i8251 USART and four drives on the
;;; debugger's sector disk. 8080 instructions only. The harness puts it at
;;; BIOS and supplies the port routines con_* and disk_*, which keep every
;;; register but A and the flags; the image builder puts it on the system
;;; tracks after CCP and BDOS.
        cpu     8080

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
        lxi     h, CCP
        mvi     c, CCP_SECTORS
        call    load_system
        jnz     wboot                   ; retry

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
        call    con_control
        call    con_control
        call    con_control             ; safest way to sync mode
        mvi     a, CMD_IR_bm
        call    con_control             ; reset
        mvi     a, ASYNC_MODE
        call    con_control
        mvi     a, RX_EN_TX_EN
        jmp     con_control

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
        call    con_status
        ani     ST_RxRDY_bm
        rz
        mvi     a, 0FFH
        ret

conin:
        call    const
        jz      conin
        call    con_in
        ani     7FH
        ret

conout:
        call    con_status
        ani     ST_TxRDY_bm
        jz      conout
        mov     a, c
        jmp     con_out

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
        jmp     disk_track

setsec:
        mov     a, c
        jmp     disk_sector

setdma:
        mov     a, c
        call    disk_dmal
        mov     a, b
        jmp     disk_dmah

;;; HL=the drive's DPH, or 0 when it has no image.
seldsk:
        lxi     h, 0
        mov     a, c
        cpi     NDISKS
        rnc
        mov     b, a
        inr     b
        call    disk_mounted
seldsk_bit:
        rrc
        dcr     b
        jnz     seldsk_bit
        rnc
        mov     a, c
        call    disk_drive
        mov     l, c                    ; HL=C*16
        dad     h
        dad     h
        dad     h
        dad     h
        lxi     d, dph0
        dad     d
;;; The drive's format picks its skew table and DPB.
        call    disk_format
        ani     0F0H                    ; the format's index
        lxi     d, xlt
        lxi     b, dpb
        jz      seldsk_dph
        lxi     d, 0                    ; hd8: no skew
        lxi     b, dpb_hd8
seldsk_dph:
        push    h
        mov     m, e                    ; XLT
        inx     h
        mov     m, d
        lxi     d, 9
        dad     d
        mov     m, c                    ; DPB
        inx     h
        mov     m, b
        pop     h
        ret

;;; A=0 when the sector moved, else 1.
read:
        mvi     a, CMD_READ
        call    disk_command
        jmp     disk_status

write:
        mvi     a, CMD_WRITE
        call    disk_command
        jmp     disk_status

;;; HL=the physical sector of logical sector BC through the table at DE,
;;; or BC itself without one.
sectran:
        mov     a, d
        ora     e
        jnz     sectran_table
        mov     h, b
        mov     l, c
        ret
sectran_table:
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

;;; hd8, 8 MB: 256 tracks of 256 sectors from 0, 8K blocks, 1016 of them,
;;; 1024 directory entries, 2 system tracks. Every CP/M shares it.
dpb_hd8:
        dw      256             ; SPT
        db      6               ; BSH
        db      63              ; BLM
        db      3               ; EXM
        dw      1015            ; DSM
        dw      1023            ; DRM
        db      0F0H            ; AL0
        db      0               ; AL1
        dw      0               ; CKS: fixed
        dw      2               ; OFF

;;; SELDSK sets each one's XLT and DPB for its drive's format.
dph0:   dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*0, alv+ALV_BYTES*0
        dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*1, alv+ALV_BYTES*1
        dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*2, alv+ALV_BYTES*2
        dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*3, alv+ALV_BYTES*3
