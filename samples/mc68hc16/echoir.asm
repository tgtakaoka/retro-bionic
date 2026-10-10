        include "mc68hc16.inc"

        include "../mc6800/mc6850.inc"

        org     $2000

rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

stack:  equ     $1000-2         ; PSHM stores at SP, then decrements it

        org     VEC_IRQ1
        fdb     isr_irq

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     (ACIA_BASE>>16)<<8 ; ZK:SK:PK: Z in the ACIA's bank
        fdb     initialize
        fdb     stack
        fdb     ACIA_BASE&$FFFF ; IZ

        org     $0200
initialize:
        clrb
        tbek                    ; data, X and Y in bank 0
        tbxk
        tbyk
        lds     #stack
        ldx     #rx_queue
        ldab    #rx_queue_size
        jsr     queue_init
        ;; initialize ACIA
        ldaa    #CDS_RESET_gc   ; Master reset
        staa    ACIA_control,z
        ldaa    #RX_INT_TX_NO
        staa    ACIA_control,z
        cli                     ; Enable IRQ
        ldx     #rx_queue
        bra     loop

wait:
        bra     loop            ; spin, IRQ enabled: it comes in running code
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
        swi                     ; halt_to_system

putchar:
        ldab    ACIA_status,z
        bitb    #TDRE_bm
        beq     putchar
        staa    ACIA_data,z
        rts

        include "../mc6801/queue.inc"

isr_irq:
        ldab    ACIA_status,z
        bitb    #IRQF_bm
        beq     isr_irq_return
isr_irq_receive:
        bitb    #RDRF_bm
        beq     isr_irq_return
        ldaa    ACIA_data,z
        ldx     #rx_queue
        jsr     queue_add
isr_irq_return:
        rti
