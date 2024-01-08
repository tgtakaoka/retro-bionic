;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Standard Call and Return Technique
;;; R0: DMA pointer
;;; R1: Program counter for Interrupt routine
;;; R2: Stack pointer
;;; R3: Program counter
;;; R4: Link register, pointer to the return location and arguments
;;;     passed by the calling program

;;; Call subroutine
;;;   SCAL R4, subroutine
;;;   DC   arguments...
;;; Subroutine return
;;;   SRET R4
;;; Return from interrupt
;;;   SEP R1

;;; Initialize for SCRT, P=0
;;; @param P!=3
;;; @param stack top address of stack
;;; @param main start address of main routine
;;; @return P=3
;;; @return R1=scrt_isr
;;; @return R2=stack
;;; @return R3=main
;;; @clobber D, R15
scrt_init:
        rldi    R3, scrt_start
        sep     R3              ; P=3
scrt_start:
        rldi    R1, scrt_isr    ; setup interrupt
        sex     R2
        rldi    R2, stack       ; setup stack
        br      main            ; goto main with P=3

;;; Interrupt exit entry P=1
;;;  (Come here by SEP R1)
scrt_isr_exit:
        irx
        rlxa    R3              ; pop program counter R3
        rlxa    R15             ; pop scratch pad register R15
        ldxa                    ; pop DF into D:MSB
        shl                     ; restore DF
        ldxa                    ; pop D
        ret                     ; restore X,P IE=1
        ;; R1 points scrt_isr
;;; CDP1802 interrupt entry, X=2, P=1, IE=0
;;; @unchanged D, DF, X, P, R3, R15
scrt_isr:
        ;; R2[0] must be preserved because it may be in the pop process
        dec     R2
        sav                     ; push X,P
        dec     R2
        stxd                    ; push D
        shrc                    ; MSB of D=DF
        stxd                    ; push DF
        rsxd    R15             ; push scratch pad register R15
        rsxd    R3              ; push program counter R3
        rldi    R3, isr
        sep     R3              ; call interrupt service routine with P=3
        br      scrt_isr_exit   ; return from isr by SEP R1
