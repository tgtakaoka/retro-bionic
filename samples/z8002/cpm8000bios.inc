;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; The CP/M-8000 BIOS for the Z8002: the console on the i8251 and four
;;; drives on the debugger's sector disk, both polled, byte I/O at
;;; 16-bit port addresses. cpm8000bios.asm links it with DRI's 1.1 kernel
;;; and the Z8002 glue of cpm8000glue.asm, pcpm8000bios.asm with the 1.2
;;; kernel built from P-CP/M-Z8K's sources; either, at SYSTEM by
;;; xout-link.py, calls the BIOS with SC #3: R3 the function, RR4 and RR6
;;; the parameters, the result back in RR6 (R7 for a word). SC #2 is the
;;; BDOS, SC #1 memory management and the context switch that starts a
;;; program. All traps go through the exception vector, 48 longs the BDOS
;;; fills with BIOS function 22, as CP/M-68K's.

DISK:   equ     150H
        include "../i8080/sector_disk.inc"
NDISKS: equ     4
;;; A Disk Parameter Header with the kernel's 16-bit pointers: XLT, three
;;; scratch words, DIRBUF, DPB, CSV and ALV.
DPH_SIZE:       equ     16
DPH_XLT:        equ     0
DPH_DPB:        equ     10
ALV_SIZE:       equ     127     ; hd8's 1016 blocks, a bit each
CSV_SIZE:       equ     16      ; the 8" disk's 64 directory entries

;;; The reset vector: the debugger serves the FCW itself. A program
;;; loaded at 0 overwrites it, so L the system again before R.
        org     ORG_RESET
        word    0
        word    FCW_SN
        word    start

;;; The Program Status Area. The debugger takes #NMI itself and serves
;;; its own vector. The vectored and non-vectored interrupt handlers
;;; leave the interrupt disabled in the interrupted FCW: nothing here
;;; asks for one.
        org     PSA
        word    0, 0
        word    FCW_SN, epu_trap        ; extended instruction
        word    FCW_SN, pi_trap         ; privileged instruction
        word    FCW_SN, sc_trap         ; system call
        word    FCW_SN, seg_trap        ; segmentation, none on a Z8002
        word    FCW_SN, nmi_trap
        word    FCW_SN, isr_nvi
        word    FCW_SN                  ; vectored interrupts: a PC an
        word    isr_vi, isr_vi, isr_vi, isr_vi, isr_vi, isr_vi, isr_vi, isr_vi
        word    isr_vi, isr_vi, isr_vi, isr_vi, isr_vi, isr_vi, isr_vi, isr_vi

        org     PSA+PSA_SIZE
;;; Cold boot: the system stack, the PSA, the traps and the console, then
;;; the CCP, which warm boots if it ever returns. The CCP initialises the
;;; BDOS only once, by a flag after autost it sets; cleared, a reset
;;; without a reload boots too.
start:
        di      vi, nvi
        ld      r15, #SYSSTK
        ld      r0, #PSA
        ldctl   psap, r0
        clrb    _autost+1
        call    trapinit
        call    biosinit
        push    @r15, #wboot
        jp      ccp

;;; The exception vector: cleared, then the memory management and BIOS
;;; system calls, and the extended instruction trap no EPU takes.
trapinit:
        ld      r2, #trapvec
        ld      r0, #NTRAPS
        subl    rr4, rr4
trapinit_clear:
        ldl     @r2, rr4
        inc     r2, #4
        djnz    r0, trapinit_clear
        clr     r2
        ld      r3, #memsc
        ldl     trapvec+4*(SC0TRAP+MEM_SC), rr2
        ld      r3, #biossc
        ldl     trapvec+4*(SC0TRAP+BIOS_SC), rr2
        ld      r3, #epu_exc
        ldl     trapvec+4*EPUTRAP, rr2
        ret

;;; The console: asynchronous, 8 bits, 1 stop bit, polled. Its interrupt
;;; identifiers, which a reset leaves as a sample set them, go to 0: a
;;; program runs with interrupts enabled.
biosinit:
        clrb    rl0
        outb    USARTC, rl0
        outb    USARTC, rl0
        outb    USARTC, rl0     ; safest way to sync mode
        ldb     rl0, #CMD_IR_bm
        outb    USARTC, rl0     ; reset
        nop
        nop
        ldb     rl0, #ASYNC_MODE
        outb    USARTC, rl0
        nop
        nop
        ldb     rl0, #RX_EN_TX_EN
        outb    USARTC, rl0
        clrb    rl0
        outb    USARTRV, rl0
        outb    USARTTV, rl0
        ret

;;; The trap dispatcher. Each trap pushes its number below what the CPU
;;; pushed, then saves R0-R14 under it and swaps it for the normal R15,
;;; the frame of cpm8000.inc, and calls the exception vector's handler
;;; with the frame at CO(R15); a zero vector resumes the program. A
;;; system call's number comes from the SC instruction word the CPU
;;; pushed.
sc_trap:
        push    @r15, #7F00H
        jr      trap
epu_trap:
        push    @r15, #EPUTRAP
        jr      trap
pi_trap:
        push    @r15, #PITRAP
        jr      trap
seg_trap:
        push    @r15, #SEGTRAP
        jr      trap
nmi_trap:
        push    @r15, #NMITRAP
trap:
        sub     r15, #FR_NR15
        ldm     @r15, r0, #15
        ldctl   r1, nsp
        ex      r1, FR_NR15(r15)
        cpb     rh1, #7FH
        jr      ne, trap_disp
        ld      r1, FR_ID(r15)  ; SC #n: 7F00H|n
        clrb    rh1
        add     r1, #SC0TRAP
trap_disp:
        sll     r1, #2
        ldl     rr0, trapvec(r1)
        testl   rr0
        jr      z, trap_ret
        call    @r1
trap_ret:
        ld      r1, FR_NR15(r15)
        ldctl   nsp, r1
        ldm     r0, @r15, #15
        add     r15, #FR_ID
        iret

;;; Interrupts: disable the kind in the interrupted FCW and return.
isr_vi:
        res     2(r15), #12     ; VIE
        iret
isr_nvi:
        res     2(r15), #11     ; NVIE
        iret

;;; SC #3: the BIOS function R3 with R5 (RR4) and RR6, its RR6 to the
;;; caller's.
biossc:
        cp      r3, #NFUNCS
        jr      uge, biossc_ret
        sll     r3, #1
        ld      r1, functions(r3)
        call    @r1
        ldl     FR_R6+CO(r15), rr6
biossc_ret:
        ret

;;; SC #1: with RR2 the length, mem_cpy copies RR2 bytes from RR6 to RR4,
;;; each a segmented address seg<<16|offset, through the SPLIT copy port.
;;; With RR2 zero, map_adr returns RR6 in the segment the space R5 names:
;;; 2 and 3 the system; 4 and 5 (and 0, 1 from a user caller) the running
;;; program's data and code; -1 sets the program's code segment from the
;;; region the loader passes, -2 is the context switch.
memsc:
        testl   rr2
        jr      z, mem_map
        outb    SPLIT_SRCSEG, rl6       ; rr6 source, seg in R6, offset R7
        outb    SPLIT_SRCL, rl7
        outb    SPLIT_SRCH, rh7
        outb    SPLIT_DSTSEG, rl4       ; rr4 dest
        outb    SPLIT_DSTL, rl5
        outb    SPLIT_DSTH, rh5
        outb    SPLIT_COUNTL, rl3       ; r3 count, 0 means 64K
        outb    SPLIT_COUNTH, rh3       ; triggers the copy
        ldl     rr6, rr4
        jr      mem_ret
mem_map:
        cp      r5, #-2
        jr      eq, xfersc
        cp      r5, #-1
        jr      eq, set_usrseg
        and     r5, #0FFH               ; drop the instruction-address bit
        cp      r5, #4                  ; TPA data
        jr      eq, mem_data
        cp      r5, #5                  ; TPA program
        jr      eq, mem_prog
        cp      r5, #2                  ; system data
        jr      eq, mem_sys
        cp      r5, #3                  ; system program
        jr      eq, mem_sys
        ld      r1, FR_FCW+CO(r15)      ; caller data or program
        bit     r1, #14                 ; a system caller stays system
        jr      nz, mem_sys
        cp      r5, #1                  ; user program
        jr      eq, mem_prog
mem_data:
        ldb     rl6, #SEG_DATA
        clrb    rh6
        jr      mem_ret
mem_prog:
        ld      r6, usrseg              ; SEG_SYS for a merged program
        jr      mem_ret
mem_sys:
        clr     r6
mem_ret:
        ldl     FR_R6+CO(r15), rr6      ; the caller's RR6
        ret
set_usrseg:
        ld      usrseg, r6              ; the region's high word is the segment
        ret

;;; The context switch starts a program: turn the split routing on for a
;;; split program (its code segment is not the system's) and off for a
;;; merged one, then copy the context block at R7, R0-R15, a word to
;;; ignore, the FCW and a long PC, over the caller's frame, which never
;;; resumes, and return into it.
xfersc:
        clrb    rl0
        ld      r2, #SEG_SYS
        ld      r1, usrseg
        cp      r1, #SEG_SYS
        jr      eq, xfer_routing
        ldb     rl0, #SPLIT_ON
        ld      r2, #SEG_DATA
xfer_routing:
        outb    SPLIT_CTRL, rl0
        ld      _usrseg, r2
        inc     r15, #CO
        ld      r5, r15
        ld      r2, #FR_ID/2
        ldir    @r5, @r7, r2
        ld      r0, CTX_FCW-FR_ID(r7)
        ld      FR_FCW(r15), r0
        ld      r0, CTX_PC-FR_ID(r7)
        ld      FR_PC(r15), r0
        jr      trap_ret

;;; What the kernel calls as C functions, the arguments on the stack
;;; above the return address, leftmost first: long bios(code, p1, p2),
;;; long bdos(code, p1), long map_adr(addr, space), mem_cpy(src, dst,
;;; len) and xfer(context).
_bios:
        ld      r3, 2(r15)
        ldl     rr4, 4(r15)
        ldl     rr6, 8(r15)
        sc      #BIOS_SC
        ret
_bdos:
        ld      r5, 2(r15)
        ldl     rr6, 4(r15)
        sc      #BDOS_SC
        ret
_map_adr:
        ldl     rr6, 2(r15)
        ld      r5, 6(r15)
        subl    rr2, rr2
        sc      #MEM_SC
        ret
_mem_cpy:
        ldl     rr6, 2(r15)
        ldl     rr4, 6(r15)
        ldl     rr2, 10(r15)
        sc      #MEM_SC
        ret
_xfer:
        ldl     rr6, 2(r15)
        ldl     rr4, #-2
        subl    rr2, rr2
        sc      #MEM_SC
        ret

;;; The BIOS functions.
functions:
        word    biosinit        ; 0 cold boot, done before the CCP
        word    wboot
        word    const
        word    conin
        word    conout
        word    list
        word    auxout
        word    auxin
        word    home
        word    seldsk
        word    settrk
        word    setsec
        word    setdma
        word    read
        word    write
        word    listst
        word    sectran
        word    unused
        word    getmrt
        word    getiob
        word    setiob
        word    flush
        word    setexc
NFUNCS: equ     ($-functions)/2

;;; CCP and BDOS stay in memory: a fresh system stack and the CCP.
wboot:
        call    biosinit
        ld      r15, #SYSSTK
        push    @r15, #wboot
        jp      ccp

;;; R7=FFH when a character is waiting, else 0.
const:
        clr     r6
        inb     rl7, USARTS
        bitb    rl7, #ST_RxRDY_bp
        ld      r7, #0
        jr      z, const_ret
        ld      r7, #0FFH
const_ret:
        ret

conin:
        inb     rl7, USARTS
        bitb    rl7, #ST_RxRDY_bp
        jr      z, conin
        inb     rl7, USARTD
        and     r7, #7FH
        clr     r6
        ret

;;; RL5 to the console.
conout:
        inb     rl0, USARTS
        bitb    rl0, #ST_TxRDY_bp
        jr      z, conout
        outb    USARTD, rl5
        ret

;;; No list or auxiliary device.
list:
auxout:
unused:
        ret
listst:
        clr     r6
        ld      r7, #0FFH       ; ready, to drop the output
        ret
auxin:
        clr     r6
        ld      r7, #1AH        ; end of file
        ret

home:
        clr     r5
settrk:
        outb    DISK_TRACK, rl5
        ret

setsec:
        outb    DISK_SECTOR, rl5
        ret

;;; RR4, the DMA address: a split program's buffer is in another segment,
;;; so keep the whole address and let read and write reach it.
setdma:
        ld      dma_seg, r4             ; R4 high word, segment in its low byte
        ld      dma_off, r5
        ret

;;; RR6=the DPH of drive R5, or 0 when it has no image or a format this
;;; BIOS lacks. DRIVE reads back the drive's format in bits 4-7, whose
;;; skew table and DPB go in its DPH.
seldsk:
        cp      r5, #NDISKS
        jr      uge, seldsk_none
        inb     rl0, DISK_MOUNTED
        bitb    rl0, r5
        jr      z, seldsk_none
        outb    DISK_DRIVE, rl5
        inb     rl0, DISK_DRIVE
        srlb    rl0, #4
        cpb     rl0, #NFORMATS
        jr      uge, seldsk_none
        clrb    rh0
        sll     r0, #2
        add     r0, #formats
        ld      r2, r0
        ld      r1, r5
        sll     r1, #4
        add     r1, #dph0
        ld      r0, @r2
        ld      DPH_XLT(r1), r0
        ld      r0, 2(r2)
        ld      DPH_DPB(r1), r0
        clr     r6
        ld      r7, r1
        ret
seldsk_none:
        subl    rr6, rr6
        ret

;;; Each format's skew table, or 0 for none, and DPB, by the index DRIVE
;;; reads back.
formats:
        word    xlt, dpb        ; 0: 8" single density, IBM 3740
        word    0, dpb_hd8      ; 1: hd8, 8 MB
NFORMATS:       equ     ($-formats)/4

;;; R7=0 when the sector moved, else 1. Every write goes through. A
;;; buffer in the system segment (a merged program, the loader) moves
;;; straight to or from the disk; one in another segment bounces through
;;; a system-segment scratch and the SPLIT copy port, since the disk
;;; reaches only the system segment.
read:
        ld      r1, dma_seg
        testb   rl1
        jr      nz, read_split
        ld      r1, dma_off             ; the system segment: DMA straight in
        outb    DISK_DMAL, rl1
        outb    DISK_DMAH, rh1
        clrb    rl1
        outb    DISK_DMAX, rl1
        ldb     rl0, #CMD_READ
        jr      disk_go
read_split:
        call    dma_scratch
        ldb     rl0, #CMD_READ
        outb    DISK_COMMAND, rl0
        inb     rl7, DISK_COMMAND
        clrb    rl6                     ; scratch (system segment) -> buffer
        outb    SPLIT_SRCSEG, rl6
        ld      r1, #scratch
        outb    SPLIT_SRCL, rl1
        outb    SPLIT_SRCH, rh1
        ld      r1, dma_seg
        outb    SPLIT_DSTSEG, rl1
        ld      r1, dma_off
        outb    SPLIT_DSTL, rl1
        outb    SPLIT_DSTH, rh1
        jr      sector_copy
write:
        ld      r1, dma_seg
        testb   rl1
        jr      nz, write_split
        ld      r1, dma_off             ; the system segment: DMA straight out
        outb    DISK_DMAL, rl1
        outb    DISK_DMAH, rh1
        clrb    rl1
        outb    DISK_DMAX, rl1
        ldb     rl0, #CMD_WRITE
        jr      disk_go
write_split:
        ld      r1, dma_seg             ; buffer -> scratch (system segment)
        outb    SPLIT_SRCSEG, rl1
        ld      r1, dma_off
        outb    SPLIT_SRCL, rl1
        outb    SPLIT_SRCH, rh1
        clrb    rl0
        outb    SPLIT_DSTSEG, rl0
        ld      r1, #scratch
        outb    SPLIT_DSTL, rl1
        outb    SPLIT_DSTH, rh1
        call    sector_count
        call    dma_scratch
        ldb     rl0, #CMD_WRITE
disk_go:
        outb    DISK_COMMAND, rl0
        inb     rl7, DISK_COMMAND
        clrb    rh7
        clr     r6
        ret
;;; The scratch holds one 128-byte sector; copy that many and trigger.
sector_copy:
        call    sector_count
disk_status:
        clrb    rh7
        clr     r6
        ret
sector_count:
        ldb     rl0, #128
        outb    SPLIT_COUNTL, rl0
        clrb    rl0
        outb    SPLIT_COUNTH, rl0       ; triggers the copy
        ret
;;; Point the disk at the scratch, in the system segment.
dma_scratch:
        ld      r1, #scratch
        outb    DISK_DMAL, rl1
        outb    DISK_DMAH, rh1
        clrb    rl1
        outb    DISK_DMAX, rl1
        ret

;;; R7=the physical sector of logical sector R5, from 0, through the
;;; table at R7, or R5 itself when R7 is 0.
sectran:
        clr     r6
        test    r7
        jr      z, sectran_none
        ld      r1, r7
        ldb     rl7, r1(r5)
        clrb    rh7
        ret
sectran_none:
        ld      r7, r5
        ret

getmrt:
        clr     r6
        ld      r7, #mrt
        ret

getiob:
        clr     r6
        ldb     rl7, iobyte
        clrb    rh7
        ret

setiob:
        ldb     iobyte, rl5
        ret

flush:
        subl    rr6, rr6
        ret

;;; Sets exception vector R5 to RR6 and returns the old one in RR6.
setexc:
        cp      r5, #NTRAPS
        jr      uge, setexc_bad
        ld      r1, r5
        sll     r1, #2
        ldl     rr2, trapvec(r1)
        ldl     trapvec(r1), rr6
        ldl     rr6, rr2
        ret
setexc_bad:
        subl    rr6, rr6
        ret

;;; An extended instruction with no EPU: skipped. The C start-up clears
;;; the Z8070's registers with them; a program that computes in floating
;;; point gets nothing back. The trap's identifier is the instruction's
;;; first word, whose mode bits say how long it is: direct and indexed
;;; addressing take a word more.
epu_exc:
        ld      r0, FR_ID+CO(r15)
        and     r0, #0C000H
        ld      r1, #4
        cp      r0, #4000H
        jr      ne, epu_skip
        ld      r1, #6
epu_skip:
        ld      r0, FR_PC+CO(r15)
        add     r0, r1
        ld      FR_PC+CO(r15), r0
        ret

;;; Skew 6, the 8" standard.
xlt:
        byte    1, 7, 13, 19, 25, 5, 11, 17, 23, 3, 9, 15, 21
        byte    2, 8, 14, 20, 26, 6, 12, 18, 24, 4, 10, 16, 22

;;; A Disk Parameter Block: SPT, BSH, BLM, EXM, a pad, DSM, DRM, AL0,
;;; AL1, CKS, OFF, then the physical sector shift and mask, 128-byte
;;; sectors here. The 8" single-density disk: 1K blocks, 243 of them, 64
;;; directory entries, 2 system tracks.
        align   2
dpb:
        word    26
        byte    3, 7, 0, 0
        word    242
        word    63
        byte    0C0H, 0
        word    16
        word    2
        byte    0, 0
;;; hd8, the debugger's 8 MB format: 256 tracks of 256 sectors from 0,
;;; no skew, 8K blocks, 1024 directory entries, 2 system tracks. The
;;; values are the format's, the same under every CP/M.
dpb_hd8:
        word    256
        byte    6, 63, 3, 0
        word    1015
        word    1023
        byte    0F0H, 0
        word    0
        word    2
        byte    0, 0

;;; SELDSK sets each DPH's skew table and DPB by the drive's format.
dph0:   word    xlt, 0, 0, 0, dirbuf, dpb, csvs+CSV_SIZE*0, alv+ALV_SIZE*0
        word    xlt, 0, 0, 0, dirbuf, dpb, csvs+CSV_SIZE*1, alv+ALV_SIZE*1
        word    xlt, 0, 0, 0, dirbuf, dpb, csvs+CSV_SIZE*2, alv+ALV_SIZE*2
        word    xlt, 0, 0, 0, dirbuf, dpb, csvs+CSV_SIZE*3, alv+ALV_SIZE*3

;;; The memory region table, by the segment in each region's high word:
;;; the merged TPA in the system segment, then a split program's 64K code
;;; and data segments, then the data view of the code segment, which the
;;; copy port reaches directly, so it is unused.
mrt:    word    4
        long    SEG_SYS*10000H, TPA_SIZE
        long    SEG_CODE*10000H, 10000H
        long    SEG_DATA*10000H, 10000H
        long    0, 0

iobyte: byte    0
;;; The kernel's: the system's segment, 0, and the program's code
;;; segment, which map_adr -1 sets.
        align   2
_sysseg:
        word    0
usrseg: word    0
;;; The running program's data segment, which the BDOS entry gives its
;;; parameters.
_usrseg:
        word    0

;;; Uninitialised areas: the exception vector, then the BDOS's scratch.
bss:    equ     $
trapvec:        equ     bss
dirbuf:         equ     trapvec+NTRAPS*4
csvs:           equ     dirbuf+128
alv:            equ     csvs+CSV_SIZE*NDISKS
;;; The disk's banked DMA target, and a system-segment scratch one sector
;;; wide that a split program's read and write bounce through.
dma_off:        equ     alv+ALV_SIZE*NDISKS
dma_seg:        equ     dma_off+2
scratch:        equ     dma_seg+2
bss_end:        equ     scratch+128
        end
