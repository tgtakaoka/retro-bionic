;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; The CP/M-8000 BIOS for the Z8001: the console on the i8251 and four
;;; drives on the debugger's sector disk, both polled, byte I/O at 16-bit
;;; port addresses. DRI's 1.1 kernel, linked at SYSTEM by xout-link.py as
;;; DRI shipped it for the Olivetti M20, runs nonsegmented in system mode
;;; in segment 0 and calls the BIOS with SC #3: R3 the function, RR4 and
;;; RR6 the parameters, the result back in RR6 (R7 for a word). SC #2 is
;;; the BDOS, SC #1 memory management and the context switch that starts
;;; a program. Traps enter segmented, as the Z8001 takes them; the
;;; dispatcher saves the M20's frame and calls each handler segmented, as
;;; the kernel's own trap glue expects. `cpu z8001` marks code that runs
;;; segmented, `cpu z8002` code that runs nonsegmented.
        cpu     z8001
        include "cpm8000.inc"
        include "../z8002/usart.inc"
        include "cpm8000sys.inc"

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

;;; The Program Status Area, at <<0>>0000H: a reserved word, an FCW and a
;;; PC's segment and offset per entry, the reset's first; the debugger
;;; serves the reset FCW and takes #NMI itself. Every handler starts
;;; segmented in system mode, interrupts disabled.
        org     PSA
        word    0, FCW_SEG|FCW_SN, 0, start
        word    0, FCW_SEG|FCW_SN, 0, epu_trap  ; extended instruction
        word    0, FCW_SEG|FCW_SN, 0, pi_trap   ; privileged instruction
        word    0, FCW_SEG|FCW_SN, 0, sc_trap   ; system call
        word    0, FCW_SEG|FCW_SN, 0, seg_trap  ; segmentation
        word    0, FCW_SEG|FCW_SN, 0, nmi_trap
        word    0, FCW_SEG|FCW_SN, 0, isr_nvi
        word    0, FCW_SEG|FCW_SN               ; vectored interrupts: a
        word    0, isr_vi, 0, isr_vi, 0, isr_vi, 0, isr_vi  ; PC each
        word    0, isr_vi, 0, isr_vi, 0, isr_vi, 0, isr_vi
        word    0, isr_vi, 0, isr_vi, 0, isr_vi, 0, isr_vi
        word    0, isr_vi, 0, isr_vi, 0, isr_vi, 0, isr_vi

;;; Cold boot, entered segmented: the system stack, the PSA, the M20's
;;; instruction space for split programs, then nonsegmented the traps and
;;; the console, and the CCP, which warm boots if it ever returns. The
;;; CCP initialises the BDOS only once, by a flag after autost it sets;
;;; cleared, a reset without a reload boots too.
        org     BIOS_ORG
start:
        di      vi, nvi
        ldl     rr14, #SYSSTK
        clr     r0
        ldctl   psapseg, r0
        ld      r0, #PSA
        ldctl   psapoff, r0
        ldb     rl0, #SPLIT_ON
        outb    SPLIT_CTRL, rl0
        ldctl   r0, fcw
        res     r0, #15
        ldctl   fcw, r0
        cpu     z8002
        clrb    _autost+1
        call    trapinit
        call    biosinit
        push    @r15, #wboot
        jp      ccp

;;; The exception vector: cleared, then the memory management and BIOS
;;; system calls, and the extended instruction trap no EPU takes, each a
;;; long address in segment 0.
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
;;; program runs with interrupts enabled, and they would hang it.
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

;;; The trap dispatcher, entered segmented. Each trap pushes its number
;;; below what the CPU pushed and saves R0-R13 under it; nonsegmented,
;;; the normal R14 and R15 go in, the number coming out for the frame of
;;; cpm8000.inc, and the exception vector's handler is called segmented
;;; with the frame at CO(R15); a zero vector resumes the program. A system
;;; call's number comes from the SC instruction word the CPU pushed. The
;;; stack is in segment 0, where nonsegmented addresses go, so the frame
;;; is reached nonsegmented.
        cpu     z8001
sc_trap:
        push    @rr14, #7F00H
        jr      trap
epu_trap:
        push    @rr14, #EPUTRAP
        jr      trap
pi_trap:
        push    @rr14, #PITRAP
        jr      trap
seg_trap:
        push    @rr14, #SEGTRAP
        jr      trap
nmi_trap:
        push    @rr14, #NMITRAP
trap:
        sub     r15, #FR_NR15
        ldm     @rr14, r0, #14
        ldctl   r0, fcw
        res     r0, #15
        ldctl   fcw, r0
        cpu     z8002
        ldctl   r1, nsp
        ld      FR_NR14(r15), r14       ; nonsegmented, R14 is the normal one
        ex      r1, FR_NR15(r15)
        cpb     rh1, #7FH
        jr      ne, trap_disp
        ld      r1, FR_ID(r15)          ; SC #n: 7F00H|n
        clrb    rh1
        add     r1, #SC0TRAP
trap_disp:
        sll     r1, #2
        ldl     rr0, trapvec(r1)
        testl   rr0
        jr      z, trap_ret
        pushl   @r15, rr0
        ldctl   r0, fcw
        set     r0, #15
        ldctl   fcw, r0
        cpu     z8001
        popl    rr0, @rr14
        calr    trap_call
        jr      trap_ret
trap_call:
        pushl   @rr14, rr0              ; jump to the handler at RR0
        ret
trap_ret:
        ldctl   r0, fcw
        res     r0, #15
        ldctl   fcw, r0
        cpu     z8002
        ld      r1, FR_NR15(r15)
        ld      r14, FR_NR14(r15)
        ldctl   nsp, r1
        ldctl   r0, fcw
        set     r0, #15
        ldctl   fcw, r0
        cpu     z8001
        ldm     r0, @rr14, #14
        add     r15, #FR_ID
        iret

;;; Interrupts: disable the kind in the interrupted FCW and return.
isr_vi:
        res     <<0>>2(r15), #12        ; VIE
        iret
isr_nvi:
        res     <<0>>2(r15), #11        ; NVIE
        iret

;;; SC #3, called segmented: the BIOS function R3 with R5 (RR4) and RR6,
;;; its RR6 to the caller's. A nonsegmented program's parameters are in
;;; its PC's segment.
biossc:
        ldctl   r0, fcw
        res     r0, #15
        ldctl   fcw, r0
        cpu     z8002
        ld      r0, FR_FCW+CO(r15)
        and     r0, #FCW_SEG|FCW_SN
        jr      nz, biossc_call
        ld      r4, FR_PCSEG+CO(r15)
        ld      r6, r4
biossc_call:
        cp      r3, #NFUNCS
        jr      uge, seg_ret
        sll     r3, #1
        ld      r1, functions(r3)
        call    @r1
        ldl     FR_R6+CO(r15), rr6
seg_ret:
        ldctl   r0, fcw
        set     r0, #15
        ldctl   fcw, r0
        cpu     z8001
        ret

;;; SC #1, called segmented. With RR2 the length, from 1 to 64K, mem_cpy
;;; copies RR2 bytes from RR6 to RR4, long addresses. With RR2 zero,
;;; map_adr returns RR6 in the segment the space R5 names: 0 and 1 the
;;; caller's data and program, 2 and 3 the system's, 4 and 5 the TPA's,
;;; a program space plus 256 meaning an instruction address rather than
;;; the data view of it; -1 sets the TPA's segment from the region the
;;; loader passes, -2 is the context switch.
memsc:
        testl   rr2
        jr      z, mem_map
        ldirb   @rr4, @rr6, r3
        ret
mem_map:
        ldctl   r0, fcw
        res     r0, #15
        ldctl   fcw, r0
        cpu     z8002
        cp      r5, #-2
        jr      eq, xfersc
        ld      r4, FR_PCSEG+CO(r15)    ; the caller's segment
        ld      r2, FR_FCW+CO(r15)      ; and FCW
        call    map_space
        ldl     FR_R6+CO(r15), rr6      ; the caller's RR6
        jr      seg_ret
map_space:
        cp      r5, #-1
        jr      eq, set_usrseg
        cpb     rl5, #0
        jr      eq, map_caller_data
        cpb     rl5, #1
        jr      eq, map_caller_prog
        cpb     rl5, #2
        jr      eq, map_sys
        cpb     rl5, #3
        jr      eq, map_sys
        cpb     rl5, #4
        jr      eq, map_tpa_data
        cpb     rl5, #5
        jr      eq, map_tpa_prog
        ret
set_usrseg:
        ld      usrseg, r6              ; the region's high word
        ret
map_caller_data:
        bit     r2, #15                 ; a segmented caller's own segment
        ret     nz
        ld      r6, r4                  ; else its PC's
        ret
map_caller_prog:
        bit     r2, #15
        jr      nz, map_prog
        ld      r6, r4
        jr      map_prog
map_sys:
        ld      r6, _sysseg
        ret
map_tpa_data:
        ld      r6, usrseg
        ret
map_tpa_prog:
        ld      r6, usrseg
;;; A split program's code, at segment 8, is in its instruction space;
;;; the BIOS reaches it as data through segment 0BH, unless the caller
;;; wants the instruction address itself.
map_prog:
        testb   rh5
        ret     nz
        and     r6, #7F00H
        cpb     rh6, #SEG_SPLIT>>8
        ret     ne
        ldb     rh6, #SEG_ISPACE>>8
        ret

;;; The context switch starts a program: copy the context block at RR6,
;;; R0-R15, a word to ignore, the FCW and the PC's segment and offset,
;;; over the caller's frame, which never resumes, and return into it.
xfersc:
        ldctl   r0, fcw
        set     r0, #15
        ldctl   fcw, r0
        cpu     z8001
        inc     r15, #CO
        ldl     rr4, rr14
        ld      r2, #FRAME/2
        ldir    @rr4, @rr6, r2
        jr      trap_ret

;;; An extended instruction with no EPU, called segmented: skipped. The C
;;; start-up clears the Z8070's registers with them; a program that
;;; computes in floating point gets nothing back. The trap's identifier is
;;; the instruction's first word, whose mode bits say how long it is:
;;; direct and indexed addressing take a word more, in segmented mode a
;;; long address two.
epu_exc:
        ldctl   r0, fcw
        res     r0, #15
        ldctl   fcw, r0
        cpu     z8002
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
        jr      seg_ret

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

;;; RR4, the DMA address, a long address whose segment the disk's 24-bit
;;; DMA takes as its high byte.
setdma:
        ldb     rl0, rh4
        andb    rl0, #7FH
        ldb     dma_seg, rl0
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

;;; R7=0 when the sector moved, else 1. Every write goes through; the
;;; disk's DMA reaches any segment.
read:
        ldb     rl0, #CMD_READ
        jr      disk_go
write:
        ldb     rl0, #CMD_WRITE
disk_go:
        ld      r1, dma_off
        outb    DISK_DMAL, rl1
        outb    DISK_DMAH, rh1
        ldb     rl1, dma_seg
        outb    DISK_DMAX, rl1
        outb    DISK_COMMAND, rl0
        inb     rl7, DISK_COMMAND
        clrb    rh7
        clr     r6
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

;;; The memory region table, the M20's, by the segment in each region's
;;; high word: a merged program's 64K, a split program's code and data,
;;; the data view of its code, and DDT's.
mrt:    word    5
        long    SEG_MERGED*10000H, 10000H
        long    SEG_SPLIT*10000H, 10000H
        long    SEG_SPLIT*10000H, 10000H
        long    SEG_ISPACE*10000H, 10000H
        long    SEG_DDT*10000H, 10000H

iobyte: byte    0
;;; The kernel's system segment, 0, and the TPA's, which map_adr -1 sets,
;;; both as a long address's high word.
        align   2
_sysseg:
        word    0
usrseg: word    0
dma_off:
        word    0
dma_seg:
        byte    0

;;; Uninitialised areas: the exception vector, then the BDOS's scratch.
        align   2
bss:    equ     $
trapvec:        equ     bss
dirbuf:         equ     trapvec+NTRAPS*4
csvs:           equ     dirbuf+128
alv:            equ     csvs+CSV_SIZE*NDISKS
bss_end:        equ     alv+ALV_SIZE*NDISKS
        end
