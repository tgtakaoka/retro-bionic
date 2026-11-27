;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Zilog's Z8070 emulator, FPE.O of CP/M-8000, on the bare Z8002: the
;;; extended instruction trap builds the frame FPE takes at fp_epu, the
;;; Olivetti M20's, below what the CPU pushed, and what FPE needs of the
;;; system follows. One address space: the program's addresses are
;;; FPE's.

;;; The frame: R0-R13, the normal R14 and R15, the trap's identifier
;;; (the instruction's first word), the FCW, the PC's segment and the PC.
FPE_R0:         equ     0
FPE_NR14:       equ     28
FPE_NR15:       equ     30
FPE_ID:         equ     32
FPE_FCW:        equ     34
FPE_PCSEG:      equ     36
FPE_PC:         equ     38
FPE_CO:         equ     4       ; FPE's stack above the frame: a word and the call

fpe_trap:
        sub     r15, #FPE_ID+2  ; the CPU's identifier, FCW and PC at FPE_ID+2
        ldm     @r15, r0, #14
        ld      FPE_NR14(r15), r14
        ldctl   r0, nsp
        ld      FPE_NR15(r15), r0
        ld      r0, FPE_ID+2(r15)
        ld      FPE_ID(r15), r0
        ld      r0, FPE_FCW+2(r15)
        ld      FPE_FCW(r15), r0
        clr     FPE_PCSEG(r15)
        push    @r15, #0
        call    fp_epu          ; runs the instruction, the PC past it
        inc     r15, #2
        ld      r0, FPE_FCW(r15)
        ld      FPE_FCW+2(r15), r0      ; the flags a comparison set
        ldm     r0, @r15, #14
        ld      r14, FPE_NR14(r15)
        add     r15, #FPE_ID+2
        iret

;;; Where the frame keeps R0, the identifier, the FCW and the PC's
;;; segment, as offsets from FPE's stack.
stareg:         word    FPE_R0+FPE_CO
staevnt:        word    FPE_ID+FPE_CO
stafcw:         word    FPE_FCW+FPE_CO
stapcsg:        word    FPE_PCSEG+FPE_CO

;;; R3=the instruction word at R7, the trapped program's PC.
gettext:
        ld      r3, @r7
        clrb    rl2
        ret

;;; R3 bytes from the program's R7 to FPE's R5, and back.
getmem:
putmem:
        test    r3
        ret     z
        push    @r15, r3
        push    @r15, r5
        push    @r15, r7
        ldirb   @r5, @r7, r3
        pop     r7, @r15
        pop     r5, @r15
        pop     r3, @r15
        ret
