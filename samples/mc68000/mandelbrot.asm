        include "mc68000.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     $1000
;;; Working space for mandelbrot.inc
F:      equ     50
vC:     ds.w    1
vD:     ds.w    1
vA:     ds.w    1
vB:     ds.w    1
vS:     ds.w    1
vP:     ds.w    1
vQ:     ds.w    1
vT:     ds.w    1
vY:     ds.w    1
vX:     ds.w    1
vI:     ds.w    1

        org     $2000
rx_queue_size:  equ     128
rx_queue:       ds.b    rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds.b    tx_queue_size

stack:  equ     $1000           ; SSP: a push decrements it first

        org     VEC_ACIA
        dc.l    isr_acia

        org     VEC_RESET
        dc.l    stack
        dc.l    initialize

        org     $0400
initialize:
        lea     (rx_queue).w,a0
        move.w  #rx_queue_size,d0
        bsr.w   queue_init
        lea     (tx_queue).w,a0
        move.w  #tx_queue_size,d0
        bsr.w   queue_init
        ;; initialize ACIA
        move.b  #CDS_RESET_gc,(ACIA_control).w ; master reset
        move.b  #RX_INT_TX_NO,(ACIA_control).w
        andi    #INTR_ON,sr     ; enable interrupts

loop:
        bsr.w   mandelbrot
        bsr.w   newline
        lea     (tx_queue).w,a0
        bsr.w   queue_wait_empty
        bra.s   loop

;;; Get character
;;; @return D0.B
;;; @return CCR.C 0 if no character
getchar:
        move.l  a0,-(a7)
        lea     (rx_queue).w,a0
        ori     #INTR_OFF,sr    ; disable interrupts
        bsr.w   queue_remove
        andi    #INTR_ON,sr     ; enable interrupts
        movea.l (a7)+,a0        ; MOVEA leaves CCR alone
        rts

;;; Put character
;;; @param D0.B
putspace:
        move.b  #' ',d0
        bra.s   putchar

newline:
        move.b  #$0D,d0
        bsr.s   putchar
        move.b  #$0A,d0
putchar:
        move.l  a0,-(a7)
        lea     (tx_queue).w,a0
putchar_retry:
        ori     #INTR_OFF,sr    ; disable interrupts
        bsr.w   queue_add
        andi    #INTR_ON,sr     ; enable interrupts
        bcc.s   putchar_retry   ; branch if queue is full
        move.b  #RX_INT_TX_INT,(ACIA_control).w ; enable Tx interrupt
        movea.l (a7)+,a0
        rts

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

isr_acia:
        movem.l d0-d1/a0,-(a7)
        move.b  (ACIA_status).w,d1
        btst    #7,d1           ; IRQF
        beq.s   isr_acia_exit
        btst    #0,d1           ; RDRF
        beq.s   isr_acia_send
        move.b  (ACIA_data).w,d0 ; receive character
        lea     (rx_queue).w,a0
        bsr.w   queue_add
isr_acia_send:
        btst    #1,d1           ; TDRE
        beq.s   isr_acia_exit
        lea     (tx_queue).w,a0
        bsr.w   queue_remove
        bcc.s   isr_acia_send_empty
        move.b  d0,(ACIA_data).w ; send character
isr_acia_exit:
        movem.l (a7)+,d0-d1/a0
        rte
isr_acia_send_empty:
        move.b  #RX_INT_TX_NO,(ACIA_control).w ; disable Tx interrupt
        movem.l (a7)+,d0-d1/a0
        rte
