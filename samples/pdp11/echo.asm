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

loop:   jsr     pc, getchar
        tstb    r0
        beq     halt_to_system
echo:   jsr     pc, putchar
        cmpb    r0, #15         ; Carriage return
        bne     loop
        movb    #12, r0         ; Line feed
        br      echo
halt_to_system:
        bpt

getchar:
        bitb    #RDRF_bm, @#ACIA_status
        beq     getchar
        movb    @#ACIA_data, r0
        rts     pc

putchar:
        bitb    #TDRE_bm, @#ACIA_status
        beq     putchar
        movb    r0, @#ACIA_data
        rts     pc
