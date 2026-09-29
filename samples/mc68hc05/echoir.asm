        include "mc68hc05c0.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FFE0
ACIA_HC05:      equ     ACIA   ; for ../mc6805/cputype.inc
        include "../mc6800/mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

        org     $50             ; RAM on the MC68HC08 too
cputype:
        rmb     1
save_a:
        rmb     1

        org     $0080
rx_queue_size:  equ     16
rx_queue:
        rmb     rx_queue_size

        org     VEC_IRQ
        fdb     isr_irq

        org     VEC_SWI
        fdb     VEC_SWI         ; halt to system

        org     VEC_RESET
        fdb     initialize

        org     $1000
initialize:
        include "../mc6805/cputype.inc"
        ldx     #rx_queue
        lda     #rx_queue_size
        jsr     queue_init
        ;; initialize ACIA
        lda     #CDS_RESET_gc   ; Master reset
        jsr     store_ACIA_control
        lda     #RX_INT_TX_NO
        jsr     store_ACIA_control
        cli                     ; Enable IRQ
        bra     loop

loop:
        lda     COP_RESET       ; service the MC68HC08's COP, a mask option
        sta     COP_RESET
        ldx     #rx_queue
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
        sta     save_a
putchar_loop:
        jsr     load_ACIA_status
        bit     #TDRE_bm
        beq     putchar_loop
putchar_data:
        lda     save_a
        jsr     store_ACIA_data
        rts

        include "../mc6805/queue.inc"

isr_irq:
        jsr     load_ACIA_status
        bit     #IRQF_bm
        beq     isr_irq_return
isr_irq_receive:
        bit     #RDRF_bm
        beq     isr_irq_recv_end
        jsr     load_ACIA_data
        ldx     #rx_queue
        jsr     queue_add
isr_irq_recv_end:
isr_irq_return:
        rti
