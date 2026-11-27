;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; The Z8002 forms of the CP/M-8000 1.1 kernel's trap glue, written over
;;; the kernel after xout-link.py places it. DRI's CPMSYS.REL is
;;; non-segmented code made for the Olivetti M20's Z8001, where traps run
;;; segmented: its BDOS entry and exception handlers read the trap frame
;;; at segmented offsets, take a 4-byte return address, dereference a
;;; segment:offset pair and toggle FCW bit 15. These routines, after the
;;; Kontron Z8002 sources of P-CP/M-Z8K 1.3 (newos/bdosif2.z8k and
;;; exceptn2.z8k), replace them at the same addresses, which the map
;;; cpm8000sys.inc gives. Everything else in the kernel is plain
;;; non-segmented code and stays.
        include "cpm8000.inc"
        include "cpm8000sys.inc"

BIOSF:          equ     50      ; BDOS 50: direct BIOS call
SETSUPF:        equ     62      ; BDOS 62: set system mode

;;; The BDOS entry, SC #2's handler through the BIOS's exception vector:
;;; R5 the function, RR6 the parameter, which for a program running
;;; split gets the data segment; the result goes back in the caller's R7.
;;; Replaces _traphnd to set_sup.
        org     _traphnd
        cp      r5, #SETSUPF
        jr      eq, set_sup
        cp      r5, #BIOSF
        jr      eq, bios_call
        ldl     rr2, rr6
        ld      r0, FR_FCW+CO(r15)
        bit     r0, #14         ; a system caller's address is physical
        jr      nz, bdos_c
        clr     r2              ; a program's: its data segment
        inb     rl0, SPLIT_CTRL
        bitb    rl0, #0
        jr      z, bdos_c
        ld      r2, #SEG_DATA
bdos_c:
        pushl   @r15, rr2       ; __bdos(func, (word)param, (long)param)
        push    @r15, r7
        push    @r15, r5
        call    __bdos
        add     r15, #8
        ld      FR_R7+CO(r15), r7
        ret
;;; R3-R7 from the parameter block at R7, then the BIOS; its RR6 goes
;;; back to the caller.
bios_call:
        ldm     r3, @r7, #5
        sc      #BIOS_SC
        ldl     FR_R6+CO(r15), rr6
        ret
set_sup:
        ld      r0, FR_FCW+CO(r15)
        set     r0, #14         ; system mode
        ld      FR_FCW+CO(r15), r0
        ret

;;; The exception handler, which each entry of the kernel's calr table
;;; at excrtn0 reaches: entry k of the table is exception 2+k for k up
;;; to 21, and 32+k-22 above. Exceptions 2-11 and 32-39 may be
;;; redirected to a program's handler through the BDOS's 18 vectors at
;;; evec_adr; the rest print the exception and warm boot. Replaces
;;; except to sysexc, before print; the link keeps _bios5 and print out
;;; of the patches' reach.
        org     except
        pop     r1, @r15        ; excrtn0 + 2k
        sub     r1, #excrtn0
        sll     r1, #1          ; 4k, a long a vector
        cp      r1, #4*9
        jr      le, exc_redir
        cp      r1, #4*22
        jr      lt, exc_default
        cp      r1, #4*29
        jr      gt, exc_default
        sub     r1, #4*12       ; vectors 10-17
exc_redir:
        ld      r2, evec_adr
        ldl     rr4, r2(r1)
        testl   rr4
        jr      nz, exc_user
        cp      r1, #4*10
        jr      lt, exc_default
        add     r1, #4*12
exc_default:
        srl     r1, #2
        add     r1, #2          ; the exception number
        cp      r1, #23
        jr      le, exc_low
        add     r1, #8
exc_low:
        push    @r15, r1
        clr     r6
        ld      r7, #excmsg1
        calr    print
        calr    prtdolla
        pop     r1, @r15
        calr    prtbyte
        clr     r6
        ld      r7, #excmsg2
        calr    print
        calr    prtdolla
        ld      r1, FR_PC+CO(r15)
        calr    prtword
        clr     r6
        ld      r7, #excmsg3
        calr    print
        clr     r5              ; BDOS 0: warm boot
        sc      #BDOS_SC
        ret
;;; A program's handler at R5 runs in the caller's mode with the frame
;;; copied onto its stack below a word standing for a return address,
;;; and exits by xfer. A system-mode caller's stack is this one: the
;;; frame is already there.
exc_user:
        ld      r0, FR_FCW+CO(r15)
        bit     r0, #14
        jr      nz, usrexc_go
        ld      r3, r15
        ld      r7, FR_NR15+CO(r15)
        sub     r7, #FRAME+CO
        ld      r6, r7
        ld      r2, #(FRAME+CO)/2
        ldir    @r7, @r3, r2
        ldctl   nsp, r6
        add     r15, #FRAME+CO+6 ; drop the frame and the CPU's
usrexc_go:
        push    @r15, r5
        push    @r15, r0
        push    @r15, #0
        iret
        end
