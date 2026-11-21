;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M-86 1.1 BIOS: the console on the i8251 USART and four drives on
;;; the debugger's sector disk. Addresses are offsets in the system
;;; segment; the Makefile places it at F0000H. The BDOS calls it with
;;; DS=CS.
        cpu     8086
        include "cpm86.inc"
        include "usart.inc"

DISK:   equ     50H
        include "../i8080/sector_disk.inc"
NDISKS: equ     4
;;; The format a drive's image has, IN DISK_DRIVE bits 4-7 once selected.
FMT_3740:       equ     0       ; 8" single density
FMT_HD8:        equ     1       ; 8 MB hard disk
ALV_BYTES:      equ     128     ; hd8's 1016 blocks

;;; The jump table's entries are 3 bytes apart; asm makes a JMP in reach
;;; short, so each is spelled out.
        org     BIOS
        db      0E9H
        dw      init-$-2
        db      0E9H
        dw      wboot-$-2
        db      0E9H
        dw      const-$-2
        db      0E9H
        dw      conin-$-2
        db      0E9H
        dw      conout-$-2
        db      0E9H
        dw      list-$-2
        db      0E9H
        dw      punch-$-2
        db      0E9H
        dw      reader-$-2
        db      0E9H
        dw      home-$-2
        db      0E9H
        dw      seldsk-$-2
        db      0E9H
        dw      settrk-$-2
        db      0E9H
        dw      setsec-$-2
        db      0E9H
        dw      setdma-$-2
        db      0E9H
        dw      read-$-2
        db      0E9H
        dw      write-$-2
        db      0E9H
        dw      listst-$-2
        db      0E9H
        dw      sectran-$-2
        db      0E9H
        dw      setdmab-$-2
        db      0E9H
        dw      getsegb-$-2
        db      0E9H
        dw      getiob-$-2
        db      0E9H
        dw      setiob-$-2

signon:
        db      0DH, 0AH, 'CP/M-86 1.1', 0DH, 0AH, 0

init:
        cli
        mov     ax, cs
        mov     ds, ax
        mov     ss, ax
        mov     sp, stack
        mov     ah, RX_EN_TX_DIS
        call    init_usart
        mov     bx, signon
        call    puts
        xor     ax, ax
        mov     es, ax
        mov     word ptr es:[BDOS_INT*4], BDOS_ENTRY
        mov     es:[BDOS_INT*4+2], cs
        mov     word ptr es:[VEC_RX*4], isr_rx
        mov     es:[VEC_RX*4+2], cs
        mov     word ptr es:[VEC_TX*4], isr_tx
        mov     es:[VEC_TX*4+2], cs
        mov     ax, cs
        mov     es, ax
        mov     byte ptr [iobyte], 0
        mov     [dma_seg], cs
        mov     cl, 0                   ; drive A:
        jmp     CCP

;;; CCP and BDOS stay in memory: no reload.
wboot:
        jmp     WARM_CCP

        include "init_usart.inc"

puts:
        mov     cl, [bx]
        or      cl, cl
        jz      puts_end
        call    conout
        inc     bx
        jmp     puts
puts_end:
        ret

;;; The USART interrupts while a character waits, and while it can
;;; take one with the transmitter on. CP/M polls with interrupts off,
;;; but a program may turn them on: the Rx handler keeps the character
;;; for CONIN, and the Tx handler turns the transmitter off, which
;;; CONOUT turns on to send.
isr_rx:
        push    ax
        in      al, USARTD
        mov     cs:[rx_char], al
        mov     byte ptr cs:[rx_ready], 0FFH
        pop     ax
        iret

isr_tx:
        push    ax
        mov     al, RX_EN_TX_DIS
        out     USARTC, al
        pop     ax
        iret

;;; Console status: AL=FFH when a character is waiting, else 0.
const:
        mov     al, [rx_ready]
        or      al, al
        jnz     const_end
        in      al, USARTS
        and     al, ST_RxRDY_bm
        jz      const_end
        mov     al, 0FFH
const_end:
        ret

conin:
        call    const
        jz      conin
        pushf
        cli
        cmp     byte ptr [rx_ready], 0
        je      conin_port
        mov     byte ptr [rx_ready], 0
        mov     al, [rx_char]
        jmp     conin_end
conin_port:
        in      al, USARTD
conin_end:
        popf
        and     al, 7FH
        ret

conout:
        pushf
        cli
        mov     al, RX_EN_TX_EN
        out     USARTC, al
conout_wait:
        in      al, USARTS
        test    al, ST_TxRDY_bm
        jz      conout_wait
        mov     al, cl
        out     USARTD, al
        popf
        ret

;;; No list, punch or reader device.
list:
punch:
        ret
listst:
        xor     al, al
        ret
reader:
        mov     al, 1AH                 ; end of file
        ret

getiob:
        mov     al, [iobyte]
        ret

setiob:
        mov     [iobyte], cl
        ret

;;; BX=the memory region table.
getsegb:
        mov     bx, mrt
        ret

home:
        xor     cx, cx
settrk:
        mov     al, cl
        out     DISK_TRACK, al
        ret

setsec:
        mov     al, cl
        out     DISK_SECTOR, al
        ret

setdma:
        mov     [dma_off], cx
        ret

setdmab:
        mov     [dma_seg], cx
        ret

;;; BX=the drive's DPH, set for its image's format, or 0 when it has
;;; no image or one of a format it does not know.
seldsk:
        xor     bx, bx
        cmp     cl, NDISKS
        jae     seldsk_end
        in      al, DISK_MOUNTED
        shr     al, cl
        test    al, 1
        jz      seldsk_end
        mov     al, cl
        out     DISK_DRIVE, al
        in      al, DISK_DRIVE
        mov     bl, cl                  ; BX=CL*16
        mov     cl, 4
        shl     bx, cl
        add     bx, dph0
        shr     al, cl
        cmp     al, FMT_3740
        je      seldsk_3740
        cmp     al, FMT_HD8
        je      seldsk_hd8
        xor     bx, bx
        ret
seldsk_3740:
        mov     word ptr [bx], xlt              ; XLT
        mov     word ptr [bx+10], dpb           ; DPB
        ret
seldsk_hd8:
        mov     word ptr [bx], 0
        mov     word ptr [bx+10], dpb_hd8
seldsk_end:
        ret

;;; AL=0 when the sector moved, else 1.
read:
        mov     ah, CMD_READ
        jmp     transfer
write:
        mov     ah, CMD_WRITE
transfer:
        mov     dx, [dma_seg]           ; DMA=dma_seg*16+dma_off
        mov     cl, 4
        rol     dx, cl
        mov     bx, dx
        and     dx, 0FFF0H
        and     bx, 000FH
        add     dx, [dma_off]
        adc     bl, 0
        mov     al, dl
        out     DISK_DMAL, al
        mov     al, dh
        out     DISK_DMAH, al
        mov     al, bl
        out     DISK_DMAX, al
        mov     al, ah
        out     DISK_COMMAND, al
        in      al, DISK_COMMAND
        ret

;;; BX=the physical sector of logical sector CX through the table at DX;
;;; without one, as hd8 has, they are the same.
sectran:
        mov     bx, cx
        or      dx, dx
        jz      sectran_end
        add     bx, dx
        mov     bl, [bx]
        xor     bh, bh
sectran_end:
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

;;; The 8 MB hard disk every CP/M shares: 256 tracks of 256 sectors from
;;; 0, 8K blocks, 1024 directory entries, 2 system tracks.
dpb_hd8:
        dw      256             ; SPT
        db      6               ; BSH
        db      63              ; BLM
        db      3               ; EXM
        dw      1015            ; DSM
        dw      1023            ; DRM
        db      0F0H            ; AL0
        db      0               ; AL1
        dw      0               ; CKS, fixed
        dw      2               ; OFF

;;; SELDSK sets each drive's XLT and DPB.
dph0:   dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*0, alv+ALV_BYTES*0
        dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*1, alv+ALV_BYTES*1
        dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*2, alv+ALV_BYTES*2
        dw      xlt, 0, 0, 0, dirbuf, dpb, csv+16*3, alv+ALV_BYTES*3

;;; One region: the TPA, up to the system segment.
mrt:    db      1
        dw      TPA_SEG, SYSSEG-TPA_SEG

iobyte: db      0
rx_ready:
        db      0
rx_char:
        db      0
dma_off:
        dw      0080H
dma_seg:
        dw      0

;;; Scratch areas.
dirbuf: ds      128
csv:    ds      16*NDISKS
alv:    ds      ALV_BYTES*NDISKS
        ds      64
stack:  equ     $

        org     0FFF0H
        jmpf    SYSSEG:init

        end
