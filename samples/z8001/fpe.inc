;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Zilog's Z8070 emulator, FPE.O of CP/M-8000, on the bare Z8001. The
;;; extended instruction trap enters segmented with the frame FPE takes
;;; at fp_epu, the Olivetti M20's, already partly pushed: the trap's
;;; identifier (the instruction's first word), the FCW and the PC's
;;; segment and offset; R0-R13 and the normal stack pointer go below.
;;; FPE runs nonsegmented, so it, this file, its data and the system
;;; stack are in segment 0, where its addresses go; what it needs of the
;;; system copies between there and the program's segments.
FPE_R0:         equ     0
FPE_NR14:       equ     28
FPE_NR15:       equ     30      ; FPE fills it in
FPE_ID:         equ     32
FPE_FCW:        equ     34
FPE_PCSEG:      equ     36
FPE_CO:         equ     4       ; the segmented call above the frame

        cpu     z8001
fpe_trap:
        sub     r15, #FPE_ID
        ldm     @rr14, r0, #14
        ldctl   r0, nspseg
        ld      rr14(#FPE_NR14), r0
        call    fp_epu          ; runs the instruction, the PC past it
        ldm     r0, @rr14, #14
        add     r15, #FPE_ID
        iret

;;; Where the frame keeps R0, the identifier, the FCW and the PC's
;;; segment, as offsets from FPE's stack.
        cpu     z8002
stareg:         word    FPE_R0+FPE_CO
staevnt:        word    FPE_ID+FPE_CO
stafcw:         word    FPE_FCW+FPE_CO
stapcsg:        word    FPE_PCSEG+FPE_CO

;;; R3=the instruction word at RR6, the trapped program's PC.
gettext:
        ldctl   r2, fcw
        set     r2, #15
        ldctl   fcw, r2
        cpu     z8001
        ld      r3, @rr6
        ldctl   r2, fcw
        res     r2, #15
        ldctl   fcw, r2
        cpu     z8002
        clrb    rl2
        ret

;;; R3 bytes from the program's RR6 to FPE's R5, in segment 0.
getmem:
        test    r3
        ret     z
        push    @r15, r3
        pushl   @r15, rr4
        pushl   @r15, rr6
        clr     r4
        jr      fpe_copy

;;; R3 bytes from FPE's R7, in segment 0, to the program's RR4.
putmem:
        test    r3
        ret     z
        push    @r15, r3
        pushl   @r15, rr4
        pushl   @r15, rr6
        clr     r6
fpe_copy:
        ldctl   r2, fcw
        set     r2, #15
        ldctl   fcw, r2
        cpu     z8001
        ldirb   @rr4, @rr6, r3
        ldctl   r2, fcw
        res     r2, #15
        ldctl   fcw, r2
        cpu     z8002
        popl    rr6, @r15
        popl    rr4, @r15
        pop     r3, @r15
        ret
