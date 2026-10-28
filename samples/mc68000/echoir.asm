        include "mc68000.inc"

        org     $2000
rx_queue_size:  equ     128
rx_queue:       ds.b    rx_queue_size
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

stack:  equ     $1000           ; SSP: a push decrements it first

        org     VEC_ACIA
        dc.l    isr_acia

        org     VEC_RESET
        dc.l    stack
        dc.l    initialize

        org     $1000
initialize:
        lea     (rx_queue).w,a0
        move.w  #rx_queue_size,d0
        bsr.w   queue_init
        ;; initialize ACIA
        move.b  #CDS_RESET_gc,(ACIA_control).w ; Master reset
        move.b  #RX_INT_TX_NO,(ACIA_control).w
        andi    #INTR_ON,sr     ; Enable interrupts
        lea     (rx_queue).w,a0

loop:
        ori     #INTR_OFF,sr    ; Disable interrupts
        bsr.w   queue_remove
        andi    #INTR_ON,sr     ; Enable interrupts
        bcc.s   loop            ; spin, interrupts on: one comes in running code
        tst.b   d0
        beq.s   halt_to_system
        bsr.s   putchar
        cmp.b   #$0D,d0
        bne.s   loop
        move.b  #$0A,d0
        bsr.s   putchar
        bra.s   loop
halt_to_system:
        illegal                 ; halt to system

putchar:
        btst    #1,(ACIA_status).w ; TDRE
        beq.s   putchar
        move.b  d0,(ACIA_data).w
        rts

        include "queue.inc"

isr_acia:
        movem.l d0-d1/a0,-(a7)
        move.b  (ACIA_status).w,d1
        btst    #7,d1           ; IRQF
        beq.s   isr_acia_return
        btst    #0,d1           ; RDRF
        beq.s   isr_acia_return
        move.b  (ACIA_data).w,d0
        lea     (rx_queue).w,a0
        bsr.w   queue_add
isr_acia_return:
        movem.l (a7)+,d0-d1/a0
        rte
