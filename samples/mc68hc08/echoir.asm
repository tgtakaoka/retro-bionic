        include "mc68hc08az0.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FFE0
        include "../mc6800/mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

        org     RAM_START

        org     $2000
rx_queue_size:  equ     128
rx_queue:
        rmb     rx_queue_size

        org     VEC_IRQ
        fdb     isr_irq

        org     VEC_SWI
        fdb     VEC_SWI         ; halt to system

        org     VEC_RESET
        fdb     initialize

        org     $1000
stack:  equ     *
initialize:
        ldhx    #stack
        txs
        ldhx    #rx_queue
        lda     #rx_queue_size
        jsr     queue_init
        ;; initialize ACIA
        lda     #CDS_RESET_gc   ; Master reset
        sta     ACIA_control
        lda     #RX_INT_TX_NO
        sta     ACIA_control
        cli                     ; Enable IRQ
        bra     loop
loop:
        ldhx    #rx_queue
        sei                     ; Disable IRQ
        jsr     queue_remove
        cli                     ; Enable IRQ
        bcc     loop
        tsta
        beq     halt_to_system
        bsr     putchar
        cmp     #$0D            ; carriage return
        bne     loop
        lda     #$0A            ; newline
        bsr     putchar
        bra     loop
halt_to_system:
        swi

putchar:
        psha
putchar_loop:
        lda     COP_RESET
        sta     COP_RESET
        lda     ACIA_status
        bit     #TDRE_bm
        beq     putchar_loop
putchar_data:
        pula
        sta     ACIA_data
        rts

        include "queue.inc"

isr_irq:
        lda     ACIA_status
        bit     #IRQF_bm
        beq     isr_irq_return
isr_irq_receive:
        bit     #RDRF_bm
        beq     isr_irq_recv_end
        lda     ACIA_data
        ldhx    #rx_queue
        jsr     queue_add
isr_irq_recv_end:
isr_irq_return:
        rti
