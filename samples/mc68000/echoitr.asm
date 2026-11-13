        include "mc68000.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

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

        org     $1000
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
        bsr.s   getchar
        bcc.s   loop            ; spin, interrupts on: one comes in running code
        tst.b   d0
        beq.s   halt_to_system
        move.b  d0,d1
        bsr.w   putchar         ; echo
        bsr.w   putspace
        bsr.w   put_hex8        ; print in hex
        bsr.w   putspace
        bsr.w   put_bin8        ; print in binary
        bsr.w   newline
        bra.s   loop
halt_to_system:
        illegal                 ; halt to system

;;; Print uint8_t in hex
;;; @param D1.B uint8_t value to be printed in hex.
;;; @clobber D0
put_hex8:
        move.b  #'0',d0
        bsr.s   putchar
        move.b  #'x',d0
        bsr.s   putchar
        move.b  d1,d0
        lsr.b   #4,d0
        bsr.s   put_hex4
        move.b  d1,d0
put_hex4:
        andi.b  #$0F,d0
        cmpi.b  #10,d0
        blo.s   put_hex8_dec
        addi.b  #'A'-10,d0
        bra.s   putchar
put_hex8_dec:
        addi.b  #'0',d0
        bra.s   putchar

;;; Print uint8_t in binary
;;; @param D1.B uint8_t value to be printed in binary.
;;; @clobber D0
put_bin8:
        move.l  d2,-(sp)
        move.b  #'0',d0
        bsr.s   putchar
        move.b  #'b',d0
        bsr.s   putchar
        moveq   #8-1,d2
put_bin8_loop:
        move.b  #'0',d0
        lsl.b   #1,d1           ; C=MSB
        bcc.s   put_bin8_put    ; MSB=0
        addq.b  #1,d0           ; MSB=1
put_bin8_put:
        bsr.s   putchar
        dbra    d2,put_bin8_loop
        move.l  (sp)+,d2
        rts

;;; Get character
;;; @return D0.B
;;; @return CCR.C 0 if no character
getchar:
        move.l  a0,-(sp)
        lea     (rx_queue).w,a0
        ori     #INTR_OFF,sr    ; disable interrupts
        bsr.w   queue_remove
        andi    #INTR_ON,sr     ; enable interrupts
        movea.l (sp)+,a0        ; MOVEA leaves CCR alone
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
        move.l  a0,-(sp)
        lea     (tx_queue).w,a0
putchar_retry:
        ori     #INTR_OFF,sr    ; disable interrupts
        bsr.w   queue_add
        andi    #INTR_ON,sr     ; enable interrupts
        bcc.s   putchar_retry   ; branch if queue is full
        move.b  #RX_INT_TX_INT,(ACIA_control).w ; enable Tx interrupt
        movea.l (sp)+,a0
        rts

        include "queue.inc"

isr_acia:
        movem.l d0-d1/a0,-(sp)
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
        movem.l (sp)+,d0-d1/a0
        rte
isr_acia_send_empty:
        move.b  #RX_INT_TX_NO,(ACIA_control).w ; disable Tx interrupt
        movem.l (sp)+,d0-d1/a0
        rte
