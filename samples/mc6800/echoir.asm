        include "mc6800.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "mc6850.inc"

        org     $2000
rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

        org     $1000
stack:  equ     *-1             ; MC6800's SP is post-decrement/pre-increment

        org     $FFF2           ; MC68HC11 IRQ
        fdb     isr_irq

        org     $FFF6           ; MC68HC11 SWI
        fdb     $FFF6

        org     VEC_IRQ
        fdb     isr_irq

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $0100
initialize:
        lds     #stack
        ldx     #rx_queue
        ldab    #rx_queue_size
        jsr     queue_init
        ;; initialize ACIA
        ldaa    #CDS_RESET_gc   ; Master reset
        staa    ACIA_control
        ldaa    #RX_INT_TX_NO
        staa    ACIA_control
        cli                     ; Enable IRQ
        ldx     #rx_queue
        bra     loop

wait:
        wai
loop:
        sei                     ; Disable IRQ
        jsr     queue_remove
        cli                     ; Enable IRQ
        bcc     wait
        tsta
        beq     halt_to_system
        bsr     putchar
        cmpa    #$0D
        bne     loop
        ldaa    #$0A
        bsr     putchar
        bra     loop
halt_to_system:
        swi                     ; halt to system

putchar:
        ldab    ACIA_status
        bitb    #TDRE_bm
        beq     putchar
        staa    ACIA_data
        rts

        include "queue.inc"

isr_irq:
        ldab    ACIA_status
        bitb    #IRQF_bm
        beq     isr_irq_return
isr_irq_receive:
        bitb    #RDRF_bm
        beq     isr_irq_return
        ldaa    ACIA_data
        ldx     #rx_queue
        jsr     queue_add
isr_irq_return:
        rti
