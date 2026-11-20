;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "../pdp11/pdp11.inc"
;;; The DCJ11 only: FP11 floating point.
        cpu     j11

;;; DL11 Asynchronous Line Interface
DL11    =       177560
DL11_VEC =      60
        include "../pdp11/dl11.inc"

        org     VEC_BPT
        .word   VEC_BPT, PSW_PRI7 ; for halt to system

        org     VEC_PWRFAIL
        .word   initialize, PSW_PRI7

        org     DL11_RVEC
        .word   isr_rx, PSW_PRI7
        org     DL11_XVEC
        .word   isr_tx, PSW_PRI7

        org     ORG_RESET
        jmp     @#initialize

        org     4000
rx_queue_size   =       128.
rx_queue:       .blkb   rx_queue_size
tx_queue_size   =       128.
tx_queue:       .blkb   tx_queue_size

        org     1000
stack   =       .
initialize:
        mov     #stack, sp
        mov     #rx_queue, r2
        mov     #rx_queue_size, r0
        jsr     pc, queue_init
        mov     #tx_queue, r2
        mov     #tx_queue_size, r0
        jsr     pc, queue_init
        ;; Initialize DL11
        movb    #RCSR_IE_bm, @#DL11_RCSR ; Enable Rx interrupt
        clrb    @#DL11_XCSR     ; Disable Tx interrupt
        mtps    #PSW_PRI0       ; Enable interrupt

loop:
        jsr     pc, dmandel
        jsr     pc, newline
        mov     #tx_queue, r2
        jsr     pc, queue_wait_empty
        br      loop

;;; Get character
;;; @return R0 character
;;; @return PSW.C 0 if no character
getchar:
        mov     r2, -(sp)
        mov     #rx_queue, r2
        mtps    #PSW_PRI7       ; Disable interrupt
        jsr     pc, queue_remove
        bcc     getchar_empty
        mtps    #PSW_PRI0       ; Enable interrupt
        mov     (sp)+, r2
        sec
        rts     pc
getchar_empty:
        mtps    #PSW_PRI0       ; Enable interrupt
        mov     (sp)+, r2
        clc
        rts     pc

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
        mov     r2, -(sp)
        mov     #tx_queue, r2
putchar_retry:
        mtps    #PSW_PRI7       ; Disable interrupt
        jsr     pc, queue_add
        bcs     putchar_added
        mtps    #PSW_PRI0       ; Enable interrupt while the queue is full
        br      putchar_retry
putchar_added:
        movb    #XCSR_IE_bm, @#DL11_XCSR ; Enable Tx interrupt
        mtps    #PSW_PRI0       ; Enable interrupt
        mov     (sp)+, r2
        rts     pc

        include "dmandel.inc"
        include "../pdp11/queue.inc"
        include "../pdp11/queue_wait.inc"

isr_rx:
        mov     r0, -(sp)
        mov     r2, -(sp)
        movb    @#DL11_RBUF, r0
        mov     #rx_queue, r2
        jsr     pc, queue_add
        mov     (sp)+, r2
        mov     (sp)+, r0
        rti

isr_tx:
        mov     r0, -(sp)
        mov     r2, -(sp)
        mov     #tx_queue, r2
        jsr     pc, queue_remove
        bcc     isr_tx_empty
        movb    r0, @#DL11_XBUF
isr_tx_exit:
        mov     (sp)+, r2
        mov     (sp)+, r0
        rti
isr_tx_empty:
        clrb    @#DL11_XCSR     ; Disable Tx interrupt
        br      isr_tx_exit
