;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "pdp11.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       177560
ACIA_VEC =      60
        include "mc6850.inc"
RX_INT_TX_NO    =       WSB_8N1_gc|RIEB_bm

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

        org     1000
stack   =       .
initialize:
        mov     #stack, sp
        mov     #rx_queue, r2
        mov     #rx_queue_size, r0
        jsr     pc, queue_init
        ;; Initialize ACIA
        movb    #CDS_RESET_gc, @#ACIA_control ; Master reset
        movb    #RX_INT_TX_NO, @#ACIA_control ; 8 bits + No Parity + 1 Stop Bits
        movb    #ACIA_VEC, @#ACIA_intr ; set the vector for MC6850 emulator
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
        bitb    #TDRE_bm, @#ACIA_status
        beq     putchar
        movb    r0, @#ACIA_data
        rts     pc

        include "queue.inc"

isr_acia:
        mov     r0, -(sp)
        mov     r2, -(sp)
        bitb    #RDRF_bm, @#ACIA_status
        beq     isr_acia_exit
        movb    @#ACIA_data, r0
        mov     #rx_queue, r2
        jsr     pc, queue_add
isr_acia_exit:
        mov     (sp)+, r2
        mov     (sp)+, r0
        rti
