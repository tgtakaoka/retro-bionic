;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "pdp11.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       177560
        include "mc6850.inc"

        org     VEC_BPT
        .word   VEC_BPT, PSW_PRI7 ; for halt to system

        org     VEC_PWRFAIL
        .word   initialize, PSW_PRI7

        org     ORG_RESET
        jmp     @#initialize

        org     1000
stack   =       .
initialize:
        mov     #stack, sp
        ;; Initialize ACIA
        movb    #CDS_RESET_gc, @#ACIA_control ; Master reset
        movb    #WSB_8N1_gc, @#ACIA_control ; 8 bits + No Parity + 1 Stop Bits
        jsr     pc, arith
        bpt                     ; halt to system

;;; Put newline
newline:
        mov     r0, -(sp)
        movb    #15, r0
        jsr     pc, putchar
        movb    #12, r0
        jsr     pc, putchar
        mov     (sp)+, r0
        rts     pc

;;; Put space
;;; @clobber R0
putspace:
        movb    #' , r0
;;; Put character
;;; @param R0 character
putchar:
        bitb    #TDRE_bm, @#ACIA_status
        beq     putchar
        movb    r0, @#ACIA_data
        rts     pc

;;; Print "R1 op R2"
;;; @param R0 op letter
;;; @clobber R0
expr:
        mov     r0, -(sp)
        mov     r1, r0
        jsr     pc, print_int16
        jsr     pc, putspace
        mov     (sp)+, r0
        jsr     pc, putchar     ; print op
        jsr     pc, putspace
        mov     r2, r0
        jmp     print_int16

;;; Print " = R1\n"
;;; @clobber R0
answer:
        jsr     pc, putspace
        movb    #'=, r0
        jsr     pc, putchar
        jsr     pc, putspace
        mov     r1, r0
        jsr     pc, print_int16
        br      newline

;;; Print "R1 rel R2\n"
;;; @clobber R0
comp:
        cmp     r1, r2
        beq     comp_eq
        blt     comp_lt
        bgt     comp_gt
        movb    #'?, r0
        br      comp_out
comp_gt:
        movb    #'>, r0
        br      comp_out
comp_eq:
        movb    #'=, r0
        br      comp_out
comp_lt:
        movb    #'<, r0
comp_out:
        jsr     pc, expr
        br      newline

arith:
        mov     #18000., r1
        mov     #28000., r2
        movb    #'+, r0
        jsr     pc, expr
        add     r2, r1          ; R1+=R2
        jsr     pc, answer      ; -19536

        mov     #18000., r1
        mov     #-18000., r2
        movb    #'+, r0
        jsr     pc, expr
        add     r2, r1          ; R1+=R2
        jsr     pc, answer      ; 0

        mov     #-18000., r1
        mov     #-18000., r2
        movb    #'+, r0
        jsr     pc, expr
        add     r2, r1          ; R1+=R2
        jsr     pc, answer      ; 29536

        mov     #-18000., r1
        mov     #-28000., r2
        movb    #'-, r0
        jsr     pc, expr
        sub     r2, r1          ; R1-=R2
        jsr     pc, answer      ; 10000

        mov     #18000., r1
        mov     #-18000., r2
        movb    #'-, r0
        jsr     pc, expr
        sub     r2, r1          ; R1-=R2
        jsr     pc, answer      ; -29536

        mov     #-28000., r1
        mov     #-18000., r2
        movb    #'-, r0
        jsr     pc, expr
        sub     r2, r1          ; R1-=R2
        jsr     pc, answer      ; -10000

        mov     #100., r1
        mov     #300., r2
        movb    #'*, r0
        jsr     pc, expr
        jsr     pc, mul16
        jsr     pc, answer      ; 30000

        mov     #200., r1
        mov     #100., r2
        movb    #'*, r0
        jsr     pc, expr
        jsr     pc, mul16
        jsr     pc, answer      ; 20000

        mov     #300., r1
        mov     #-200., r2
        movb    #'*, r0
        jsr     pc, expr
        jsr     pc, mul16
        jsr     pc, answer      ; 5536

        mov     #100., r1
        mov     #-300., r2
        movb    #'*, r0
        jsr     pc, expr
        jsr     pc, mul16
        jsr     pc, answer      ; -30000

        mov     #-200., r1
        mov     #-100., r2
        movb    #'*, r0
        jsr     pc, expr
        jsr     pc, mul16
        jsr     pc, answer      ; 20000

        mov     #30000., r1
        mov     #100., r2
        movb    #'/, r0
        jsr     pc, expr
        jsr     pc, div16
        jsr     pc, answer      ; 300

        mov     #-200., r1
        mov     #100., r2
        movb    #'/, r0
        jsr     pc, expr
        jsr     pc, div16
        jsr     pc, answer      ; -2

        mov     #-30000., r1
        mov     #-200., r2
        movb    #'/, r0
        jsr     pc, expr
        jsr     pc, div16
        jsr     pc, answer      ; 150

        mov     #-30000., r1
        mov     #78., r2
        movb    #'/, r0
        jsr     pc, expr
        jsr     pc, div16
        jsr     pc, answer      ; -384

        mov     #5000., r1
        mov     #4000., r2
        jsr     pc, comp

        mov     #5000., r1
        mov     #5000., r2
        jsr     pc, comp

        mov     #4000., r1
        mov     #5000., r2
        jsr     pc, comp

        mov     #-5000., r1
        mov     #-4000., r2
        jsr     pc, comp

        mov     #-5000., r1
        mov     #-5000., r2
        jsr     pc, comp

        mov     #-4000., r1
        mov     #-5000., r2
        jsr     pc, comp

        mov     #32700., r1
        mov     #32600., r2
        jsr     pc, comp

        mov     #32700., r1
        mov     #32700., r2
        jsr     pc, comp

        mov     #32600., r1
        mov     #32700., r2
        jsr     pc, comp

        mov     #-32700., r1
        mov     #-32600., r2
        jsr     pc, comp

        mov     #-32700., r1
        mov     #-32700., r2
        jsr     pc, comp

        mov     #-32600., r1
        mov     #-32700., r2
        jsr     pc, comp

        mov     #18000., r1
        mov     #-28000., r2
        jsr     pc, comp

        mov     #-28000., r1
        mov     #-28000., r2
        jsr     pc, comp

        mov     #-28000., r1
        mov     #18000., r2
        jsr     pc, comp
        rts     pc

        include "arith.inc"
