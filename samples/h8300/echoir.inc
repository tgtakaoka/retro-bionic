;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Echo what the ACIA receives, its receive interrupt queueing each
;;; character at rx_queue; CR is echoed as CR LF. NUL halts.
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

;;; Initialize the queue and the ACIA; IRQ stays masked
;;; @clobber R0 R1
init_echoir:
        mov.w   #rx_queue, r1
        mov.b   #rx_queue_size, r0l
        jsr     @queue_init
        mov.b   #CDS_RESET_gc, r0l
        mov.b   r0l, @ACIA_control:8 ; master reset
        mov.b   #RX_INT_TX_NO, r0l
        mov.b   r0l, @ACIA_control:8
        rts

echoir_wait:
        bra     echoir_loop     ; spin, IRQ enabled: it comes in running code
echoir_loop:
        mov.w   #rx_queue, r1
        orc     #CCR_I, ccr     ; disable IRQ
        jsr     @queue_remove
        andc    #~CCR_I&$FF, ccr ; enable IRQ
        bcc     echoir_wait
        mov.b   r0l, r0l
        beq     echoir_halt
echoir_char:
        btst    #1, @ACIA_status:8 ; TDRE
        beq     echoir_char
        mov.b   r0l, @ACIA_data:8
        cmp.b   #$0D, r0l
        bne     echoir_loop
        mov.b   #$0A, r0l
        bra     echoir_char
echoir_halt:
        jmp     @halt_to_system

isr_irq:
        push    r0              ; an interrupt stacks only PC and CCR
        push    r1
        btst    #7, @ACIA_status:8 ; IRQF
        beq     isr_irq_return
        btst    #0, @ACIA_status:8 ; RDRF
        beq     isr_irq_return
        mov.b   @ACIA_data:8, r0l
        mov.w   #rx_queue, r1
        jsr     @queue_add
isr_irq_return:
        pop     r1
        pop     r0
        rte

        include "queue.inc"

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
