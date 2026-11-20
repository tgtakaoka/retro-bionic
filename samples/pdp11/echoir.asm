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

        org     DL11_RVEC
        .word   isr_rx, PSW_PRI7

        org     ORG_RESET
        jmp     @#initialize

        org     4000
rx_queue_size   =       128.
rx_queue:       .blkb   rx_queue_size

        org     1000
stack   =       .
initialize:
        mov     #stack, sp
        mov     #rx_queue, r2
        mov     #rx_queue_size, r0
        jsr     pc, queue_init
        ;; Initialize DL11
        movb    #RCSR_IE_bm, @#DL11_RCSR ; Enable Rx interrupt
        clrb    @#DL11_XCSR     ; Disable Tx interrupt
        mtps    #PSW_PRI0       ; Enable interrupt

loop:   jsr     pc, getchar
        bcc     loop
        tstb    r0
        beq     halt_to_system
echo:   jsr     pc, putchar
        cmpb    r0, #15         ; Carriage return
        bne     loop
        movb    #12, r0         ; Line feed
        br      echo
halt_to_system:
        bpt

;;; Get character
;;; @return R0 character
;;; @return PSW.C 0 if no character
;;; @clobber R2
getchar:
        mov     #rx_queue, r2
        mtps    #PSW_PRI7       ; Disable interrupt
        jsr     pc, queue_remove
        bcc     getchar_empty
        mtps    #PSW_PRI0       ; Enable interrupt
        sec
        rts     pc
getchar_empty:
        mtps    #PSW_PRI0       ; Enable interrupt
        clc
        rts     pc

putchar:
        bitb    #XCSR_READY_bm, @#DL11_XCSR
        beq     putchar
        movb    r0, @#DL11_XBUF
        rts     pc

        include "queue.inc"

isr_rx:
        mov     r0, -(sp)
        mov     r2, -(sp)
        movb    @#DL11_RBUF, r0
        mov     #rx_queue, r2
        jsr     pc, queue_add
        mov     (sp)+, r2
        mov     (sp)+, r0
        rti
