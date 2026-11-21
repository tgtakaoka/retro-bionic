;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M-68K 1.3 BIOS: the console on the MC6850 ACIA and four drives on
;;; the debugger's sector disk, both reached by byte so the 8-bit-bus
;;; boards run it too. The BDOS calls TRAP #3 with the function in D0.W
;;; and its arguments in D1 and D2; the result comes back in D0. D1-D2
;;; and A0-A1 are scratch, as in DRI's C BIOS.
        include "mc68000.inc"
        include "cpm68k.inc"

DISK:   equ     $FFFFE0
        include "../i8080/sector_disk.inc"
NDISKS: equ     4
DPH_SIZE:       equ     26
DPH_XLT:        equ     0
DPH_DPB:        equ     14
ALV_SIZE:       equ     127     ; hd8's 1016 blocks, a bit each
VEC_TRAP3:      equ     VEC_TRAP0+3*4

        org     VEC_RESET
        dc.l    BIOS            ; SSP until the CCP sets its own
        dc.l    CPM             ; L zeroed CCP and BDOS's BSS

;;; CCP calls it once, from the cold start, before the BDOS. D0=0 logs
;;; in drive A: as user 0.
        org     BIOS
init:
        move.l  #trap3,(VEC_TRAP3).w
        move.b  #CDS_RESET_gc,(ACIA_control).w
        move.b  #WSB_8N1_gc,(ACIA_control).w
        lea     (signon,pc),a0
init_signon:
        move.b  (a0)+,d1
        beq.s   init_end
        bsr.w   conout
        bra.s   init_signon
init_end:
        clr.b   (iobyte).l
        bsr.s   sizemem
        sub.l   #TPA_GAP+TPA,d1
        move.l  d1,(mrt_size).l
        moveq   #0,d0
        rts

;;; D1=the size of the address space: where a write past TPA lands back
;;; on TPA, 1 MB on the MC68008P, 4 MB on the MC68008FN, else 16 MB, and
;;; at most MEM_MAX.
sizemem:
        lea     (TPA).l,a0
        move.l  #$100000,d1
sizemem_loop:
        lea     (a0,d1.l),a1
        move.l  #$43504D31,(a0)
        move.l  #$43504D32,(a1)
        cmpi.l  #$43504D32,(a0)
        beq.s   sizemem_end
        lsl.l   #2,d1
        cmp.l   #$1000000,d1
        bcs.s   sizemem_loop
sizemem_end:
        cmp.l   #MEM_MAX,d1
        bls.s   sizemem_max
        move.l  #MEM_MAX,d1
sizemem_max:
        rts


signon:
        dc.b    $0D, $0A, 'CP/M-68K 1.3', $0D, $0A, 0
        align   2

trap3:
        cmp.w   #NFUNCS,d0
        bcc.s   trap3_end
        add.w   d0,d0
        add.w   d0,d0
        movea.l (functions,pc,d0.w),a0
        jsr     (a0)
trap3_end:
        rte

functions:
        dc.l    init
        dc.l    wboot
        dc.l    const
        dc.l    conin
        dc.l    conout
        dc.l    list
        dc.l    auxout
        dc.l    auxin
        dc.l    home
        dc.l    seldsk
        dc.l    settrk
        dc.l    setsec
        dc.l    setdma
        dc.l    read
        dc.l    write
        dc.l    listst
        dc.l    sectran
        dc.l    unused
        dc.l    getmrt
        dc.l    getiob
        dc.l    setiob
        dc.l    flush
        dc.l    setexc
NFUNCS: equ     (*-functions)/4

;;; CCP and BDOS stay in memory: no reload.
wboot:
        jmp     (CCP).l

;;; Console status: D0=FFH when a character is waiting, else 0.
const:
        moveq   #0,d0
        btst    #0,(ACIA_status).w      ; RDRF
        beq.s   const_end
        move.l  #$FF,d0
const_end:
        rts

conin:
        btst    #0,(ACIA_status).w      ; RDRF
        beq.s   conin
        moveq   #$7F,d0
        and.b   (ACIA_data).w,d0
        rts

conout:
        btst    #1,(ACIA_status).w      ; TDRE
        beq.s   conout
        move.b  d1,(ACIA_data).w
        rts

;;; No list, auxiliary or I/O byte device.
list:
auxout:
unused:
        rts
listst:
        move.l  #$FF,d0                 ; ready, to drop the output
        rts
auxin:
        moveq   #$1A,d0                 ; end of file
        rts

getiob:
        moveq   #0,d0
        move.b  (iobyte).l,d0
        rts

setiob:
        move.b  d1,(iobyte).l
        rts

;;; Every sector is written through.
flush:
        moveq   #0,d0
        rts

;;; D0=the memory region table.
getmrt:
        move.l  #mrt,d0
        rts

;;; Sets exception vector D1.W to D2 and returns the old one in D0.
setexc:
        and.l   #$FF,d1
        add.w   d1,d1
        add.w   d1,d1
        movea.l d1,a0
        move.l  (a0),d0
        move.l  d2,(a0)
        rts

home:
        moveq   #0,d1
settrk:
        move.b  d1,(DISK_TRACK).w
        rts

setsec:
        move.b  d1,(DISK_SECTOR).w
        rts

setdma:
        move.b  d1,(DISK_DMAL).w
        lsr.l   #8,d1
        move.b  d1,(DISK_DMAH).w
        lsr.l   #8,d1
        move.b  d1,(DISK_DMAX).w
        rts

;;; D0=the DPH of drive D1.B, or 0 when it has no image or a format
;;; this BIOS lacks. DRIVE reads back the drive's format in bits 4-7,
;;; whose DPB and skew go in its DPH.
seldsk:
        moveq   #0,d0
        cmp.b   #NDISKS,d1
        bcc.s   seldsk_end
        btst    d1,(DISK_MOUNTED).w
        beq.s   seldsk_end
        move.b  d1,(DISK_DRIVE).w
        move.b  (DISK_DRIVE).w,d2
        lsr.b   #4,d2
        cmp.b   #NFORMATS,d2
        bcc.s   seldsk_end
        ext.w   d2
        lsl.w   #3,d2
        lea     (formats,pc,d2.w),a1
        move.b  d1,d0
        mulu.w  #DPH_SIZE,d0
        add.l   #dph0,d0
        movea.l d0,a0
        move.l  (a1)+,(DPH_XLT,a0)
        move.l  (a1),(DPH_DPB,a0)
seldsk_end:
        rts

;;; Each format's skew table, or 0 for none, and DPB, by the index DRIVE
;;; reads back.
formats:
        dc.l    xlt, dpb        ; 0: 8" single density, IBM 3740
        dc.l    0, dpb_hd8      ; 1: hd8, 8 MB
NFORMATS:       equ     (*-formats)/8

;;; D0=0 when the sector moved, else 1.
read:
        move.b  #CMD_READ,(DISK_COMMAND).w
        bra.s   status
write:
        move.b  #CMD_WRITE,(DISK_COMMAND).w
status:
        moveq   #0,d0
        tst.b   (DISK_COMMAND).w
        beq.s   status_end
        moveq   #1,d0
status_end:
        rts

;;; D0=the physical sector of logical sector D1.W, from 0, through the
;;; table at D2, or D1 itself when D2 is 0.
sectran:
        moveq   #0,d0
        move.w  d1,d0
        tst.l   d2
        beq.s   sectran_end
        movea.l d2,a0
        move.b  (a0,d1.w),d0
sectran_end:
        rts

;;; Skew 6, the 8" standard.
xlt:
        dc.b    1, 7, 13, 19, 25, 5, 11, 17, 23, 3, 9, 15, 21
        dc.b    2, 8, 14, 20, 26, 6, 12, 18, 24, 4, 10, 16, 22

;;; 8" single-density disk parameters: 1K blocks, 243 of them, 64
;;; directory entries, 2 system tracks.
dpb:
        dc.w    SPT             ; SPT, sectors a track
        dc.b    3               ; BSH, block shift
        dc.b    7               ; BLM, block mask
        dc.b    0               ; EXM, extent mask
        dc.b    0
        dc.w    242             ; DSM, last block
        dc.w    63              ; DRM, last directory entry
        dc.b    $C0             ; AL0, the directory's blocks
        dc.b    0               ; AL1
        dc.w    16              ; CKS, directory check size
        dc.w    2               ; OFF, system tracks

;;; hd8, the debugger's 8 MB format: 256 tracks of 256 sectors from 0,
;;; no skew, 8K blocks, 1024 directory entries, 2 system tracks. The
;;; values are the format's, the same under every CP/M.
dpb_hd8:
        dc.w    256             ; SPT
        dc.b    6               ; BSH
        dc.b    63              ; BLM
        dc.b    3               ; EXM
        dc.b    0
        dc.w    1015            ; DSM
        dc.w    1023            ; DRM
        dc.b    $F0             ; AL0
        dc.b    0               ; AL1
        dc.w    0               ; CKS, a fixed disk
        dc.w    2               ; OFF

;;; SELDSK sets each DPH's skew and DPB by the drive's format.
dph0:   dc.l    xlt
        dc.w    0, 0, 0
        dc.l    dirbuf, dpb, csv+16*0, alv+ALV_SIZE*0
        dc.l    xlt
        dc.w    0, 0, 0
        dc.l    dirbuf, dpb, csv+16*1, alv+ALV_SIZE*1
        dc.l    xlt
        dc.w    0, 0, 0
        dc.l    dirbuf, dpb, csv+16*2, alv+ALV_SIZE*2
        dc.l    xlt
        dc.w    0, 0, 0
        dc.l    dirbuf, dpb, csv+16*3, alv+ALV_SIZE*3

;;; One region: the TPA, sized at cold boot.
mrt:    dc.w    1
        dc.l    TPA
mrt_size:
        dc.l    0

;;; Scratch areas.
dirbuf: ds.b    128
csv:    ds.b    16*NDISKS
alv:    ds.b    ALV_SIZE*NDISKS
iobyte: ds.b    1
bios_end:

        end
