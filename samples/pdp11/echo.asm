;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "pdp11.inc"

;;; DL11 Asynchronous Line Interface
DL11    =       177560
DL11_VEC =      60
        include "dl11.inc"

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
        ;; Initialize DL11
        clrb    @#DL11_RCSR     ; Disable Rx interrupt
        clrb    @#DL11_XCSR     ; Disable Tx interrupt

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
        bitb    #RCSR_DONE_bm, @#DL11_RCSR
        beq     getchar
        movb    @#DL11_RBUF, r0
        rts     pc

putchar:
        bitb    #XCSR_READY_bm, @#DL11_XCSR
        beq     putchar
        movb    r0, @#DL11_XBUF
        rts     pc
