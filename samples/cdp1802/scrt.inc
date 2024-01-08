;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Standard Call and Return Technique
;;; R0: DMA pointer
;;; R1: Program counter for Interrupt routine
;;; R2: Stack pointer
;;; R3: Program counter
;;; R4: Link register, pointer to the return location and arguments
;;;     passed by the calling program
;;; R5: Dedicated program counter for CALL routine
;;; R6: Dedicated program counter for RETURN routine

;;; Call subroutine
;;;   SEP R5
;;;   DC  A(subroutine)
;;;   DC  arguments...
;;; Subroutine return
;;;   SEP R6
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
;;; @return R5=scrt_call
;;; @return R6=scrt_return
;;; @clobber D, R15
scrt_init:
        ldi     A.1(scrt_start)
        phi     R3
        ldi     A.0(scrt_start)
        plo     R3
        sep     R3              ; P=3
scrt_start:
        ldi     A.1(scrt_init_tab)
        phi     R15
        ldi     A.0(scrt_init_tab)
        plo     R15
        lda     R15             ; setup interrupt
        phi     R1
        lda     R15
        plo     R1
        lda     R15             ; setup stack
        phi     R2
        lda     R15
        plo     R2
        lda     R15             ; setup call
        phi     R5
        lda     R15
        plo     R5
        lda     R15             ; setup return
        phi     R6
        lda     R15
        plo     R6
        br      main            ; goto main with P=3

scrt_init_tab:
        dc      A(scrt_isr)     ; R1
        dc      A(stack)        ; R2
        dc      A(scrt_call)    ; R5
        dc      A(scrt_return)  ; R6

;;; Call subroutine, P=5
;;; @param M(R3):M(R3+1) subroutine address
;;; @param M(R3+2) optional arguments, depending on a subroutine called.
;;; @return R4 points optional arguments
;;; @return M(R2) R4.1, R4.0
;;; @clobber R15.1, X
;;; @unchanged D, DF, IE, R0, R1, R7-R14, R15.0
scrt_call_exit:
        sep     R3              ; go to subroutine
scrt_call:
        phi     R15             ; save D to R15.1
        sex     R2              ; select stack
        glo     R4              ; push old link register R4
        stxd
        ghi     R4
        stxd
        ghi     R3              ; load link register
        phi     R4
        glo     R3
        plo     R4
        lda     R4              ; load the address of subroutine
        phi     R3
        lda     R4
        plo     R3              ; R3=subroutine address
        ghi     R15             ; restore D
        br      scrt_call_exit

;;; Return subroutine, P=6
;;; @param R4 return address
;;; @oaram M(R2) R4.1, R4.0, X:P(3)
;;; @clobber R15.1, X
;;; @unchanged D, DF, R0, R1, R7-R14, R15.0
scrt_return_exit:
        sep     R3              ; return to subroutine caller
scrt_return:
        phi     R15             ; save D to R15.1
        ghi     R4              ; load return address from link register
        phi     R3
        glo     R4
        plo     R3
        sex     R2              ; select stack
        irx
        ldxa                    ; pop link register R4
        phi     R4
        ldx
        plo     R4
        ghi     R15             ; restore D
        br      scrt_return_exit

;;; Interrupt exit entry P=1
;;;  (Come here by SEP R1)
scrt_isr_exit:
        irx
        ldxa                    ; pop program counter R3
        phi     R3
        ldxa
        plo     R3
        ldxa                    ; pop call pointer R5
        phi     R5
        ldxa
        plo     R5
        ldxa                    ; pop return pointer R6
        phi     R6
        ldxa
        plo     R6
        ldxa                    ; pop scratch pad register R15
        phi     R15
        ldxa
        plo     R15
        ldxa                    ; pop DF into D:MSB
        shl                     ; restore DF
        ldxa                    ; pop D
        ret                     ; restore X,P IE=1
        ;; R1 points scrt_isr
;;; CDP1802 interrupt entry, X=2, P=1, IE=0
;;; @unchanged D, DF, X, P, R3, R5, R6, R15
scrt_isr:
        ;; R2[0] must be preserved because it may be in the pop process
        dec     R2
        sav                     ; push X,P
        dec     R2
        stxd                    ; push D
        shrc                    ; MSB of D=DF
        stxd                    ; push DF
        glo     R15             ; push scratch pad register R15
        stxd
        ghi     R15
        stxd
        glo     R6              ; push return pointer R6
        stxd
        ghi     R6
        stxd
        glo     R5              ; push call pointer R5
        stxd
        ghi     R5
        stxd
        glo     R3              ; push program counter R3
        stxd
        ghi     R3
        stxd
        ;; Initialize SCRT registers because interrupt service routine
        ;; may use call/return
        ldi     A.1(scrt_call)  ; initialize call pointer R5
        phi     R5
        ldi     A.0(scrt_call)
        plo     R5
        ldi     A.1(scrt_return) ; initialize return pointer R6
        phi     R6
        ldi     A.0(scrt_return)
        plo     R6
        ;;
        ldi     A.1(isr)
        phi     R3
        ldi     A.0(isr)
        plo     R3
        sep     R3              ; call interrupt service routine with P=3
        br      scrt_isr_exit   ; return from isr by SEP R1
