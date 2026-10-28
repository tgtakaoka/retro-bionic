;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "pdp11.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       177560
ACIA_VEC =      60
        include "mc6850.inc"
RX_INT_TX_NO    =       WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT   =       WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     VEC_BPT
        .word   VEC_BPT, PSW_PRI7 ; for halt to system

        org     VEC_PWRFAIL
        .word   initialize, PSW_PRI7

        org     ACIA_VEC
        .word   isr_acia, PSW_PRI7

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
        ;; Initialize ACIA
        movb    #CDS_RESET_gc, @#ACIA_control ; Master reset
        movb    #RX_INT_TX_NO, @#ACIA_control ; 8 bits + No Parity + 1 Stop Bits
        movb    #ACIA_VEC, @#ACIA_intr ; set the vector for MC6850 emulator
        mtps    #PSW_PRI0       ; Enable interrupt

loop:
        jsr     pc, mandelbrot
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
        movb    #RX_INT_TX_INT, @#ACIA_control ; Enable Tx interrupt
        mtps    #PSW_PRI0       ; Enable interrupt
        mov     (sp)+, r2
        rts     pc

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"
        include "queue_wait.inc"

isr_acia:
        mov     r0, -(sp)
        mov     r2, -(sp)
        bitb    #RDRF_bm, @#ACIA_status
        beq     isr_acia_send
        movb    @#ACIA_data, r0
        mov     #rx_queue, r2
        jsr     pc, queue_add
isr_acia_send:
        bitb    #TDRE_bm, @#ACIA_status
        beq     isr_acia_exit
        mov     #tx_queue, r2
        jsr     pc, queue_remove
        bcc     isr_acia_send_empty
        movb    r0, @#ACIA_data
isr_acia_exit:
        mov     (sp)+, r2
        mov     (sp)+, r0
        rti
isr_acia_send_empty:
        movb    #RX_INT_TX_NO, @#ACIA_control ; Disable Tx interrupt
        br      isr_acia_exit
