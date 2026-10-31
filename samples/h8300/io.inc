;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Console through the ACIA's interrupt on #IRQ0: received characters
;;; queue at rx_queue, characters to send at tx_queue.
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

;;; Initialize the queues and the ACIA; IRQ stays masked
;;; @clobber R0 R1
init_io:
        mov.w   #rx_queue, r1
        mov.b   #rx_queue_size, r0l
        jsr     @queue_init
        mov.w   #tx_queue, r1
        mov.b   #tx_queue_size, r0l
        jsr     @queue_init
        mov.b   #CDS_RESET_gc, r0l
        mov.b   r0l, @ACIA_control:8 ; master reset
        mov.b   #RX_INT_TX_NO, r0l
        mov.b   r0l, @ACIA_control:8
        rts

;;; Get character
;;; @return R0L
;;; @return CCR.C 0 if no character
getchar:
        push    r1
        mov.w   #rx_queue, r1
        orc     #CCR_I, ccr     ; disable IRQ
        jsr     @queue_remove
        andc    #~CCR_I&$FF, ccr ; enable IRQ
        pop     r1
        rts

;;; Put character
;;; @param R0L
putspace:
        mov.b   #" ", r0l
        bra     putchar
newline:
        mov.b   #$0D, r0l
        bsr     putchar
        mov.b   #$0A, r0l
putchar:
        push    r0
        push    r1
        mov.w   #tx_queue, r1
putchar_retry:
        orc     #CCR_I, ccr     ; disable IRQ
        jsr     @queue_add
        andc    #~CCR_I&$FF, ccr ; enable IRQ
        bcc     putchar_retry   ; branch if queue is full
        mov.b   #RX_INT_TX_INT, r0l
        mov.b   r0l, @ACIA_control:8 ; enable Tx interrupt
        pop     r1
        pop     r0
        rts

;;; Wait until the queue at R1 is empty
queue_wait_empty:
        mov.b   @(queue_len, r1), r0l
        bne     queue_wait_empty
        rts

isr_irq:
        push    r0              ; an interrupt stacks only PC and CCR
        push    r1
        push    r2
        mov.b   @ACIA_status:8, r2l
        btst    #7, r2l         ; IRQF
        beq     isr_irq_exit
        btst    #0, r2l         ; RDRF
        beq     isr_irq_send
        mov.b   @ACIA_data:8, r0l ; receive character
        mov.w   #rx_queue, r1
        jsr     @queue_add
isr_irq_send:
        btst    #1, r2l         ; TDRE
        beq     isr_irq_exit
        mov.w   #tx_queue, r1
        jsr     @queue_remove
        bcc     isr_irq_send_empty
        mov.b   r0l, @ACIA_data:8 ; send character
isr_irq_exit:
        pop     r2
        pop     r1
        pop     r0
        rte
isr_irq_send_empty:
        mov.b   #RX_INT_TX_NO, r0l
        mov.b   r0l, @ACIA_control:8 ; disable Tx interrupt
        bra     isr_irq_exit

        include "queue.inc"

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
