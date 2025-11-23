        include "hd6309.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "../mc6800/mc6850.inc"

        org     $2000

rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

        org     $1000
stack:  equ     *

        org     VEC_IRQ
        fdb     isr_irq

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $0100
initialize:
        ldmd    #1              ; 6309 native mode
        lds     #stack
        ldx     #rx_queue
        ldb     #rx_queue_size
        lbsr    queue_init
        ;; initialize ACIA
        lda     #CDS_RESET_gc   ; Master reset
        sta     ACIA_control
        lda     #RX_INT_TX_NO
        sta     ACIA_control
        lda     #VEC_IRQ        ; IRQ
        sta     ACIA+2          ; set #IRQ name for MC6850 emulator
        ldx     #rx_queue

wait:
        cwai    #~CC_IRQ        ; Clear IRQ mask
loop:
        orcc    #CC_IRQ         ; Set IRQ mask
        lbsr    queue_remove
        andcc   #~CC_IRQ        ; Clear IRQ mask
        bcc     wait
        tsta
        beq     halt_to_system
        bsr     putchar
        cmpa    #$0D
        bne     loop
        lda     #$0A
        bsr     putchar
        bra     loop
halt_to_system:
        swi                     ; halt to system

putchar:
        ldb     ACIA_status
        bitb    #TDRE_bm
        beq     putchar
        sta     ACIA_data
        rts

        include "../mc6809/queue.inc"

;;; IRQ
isr_irq:
        ldb     ACIA_status
        bitb    #IRQF_bm
        beq     isr_irq_return
isr_irq_receive:
        bitb    #RDRF_bm
        beq     isr_irq_return
        lda     ACIA_data
        ldx     #rx_queue
        jsr     queue_add
isr_irq_return:
        rti
