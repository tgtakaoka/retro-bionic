        cpu     6811
        include "mc68hc11d.inc"

        org     $2000

rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size

;;; SCI: Enable Rx interrupt
RX_INT_TX_NO:   equ     SCCR2_TE_bm|SCCR2_RE_bm|SCCR2_RIE_bm

        org     $1000
stack:  equ     *-1             ; MC6801's SP is post-decrement/pre-increment

        org     VEC_SCI
        fdb     isr_sci

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $0100
device_base:
        fdb     $0000
initialize:
        lds     #stack
        ldx     #rx_queue
        ldab    #rx_queue_size
        jsr     queue_init
        ;; Initialize SCI
        ldx     device_base
        clr     SCCR1,x         ; 8bit 1stop
        ldaa    #BAUD_SCP1_gc|BAUD_SCR1_gc ; E/16
        staa    BAUD,x
        ldaa    #RX_INT_TX_NO
        staa    SCCR2,x         ; Enable Tx and Rx/Interrupt
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
        swi                     ; halt_to_system

putchar:
        pshx
        ldx     device_base
;;; Transmit Data Register Empty?
        brclr   SCSR,x, #SCSR_TDRE_bm, putchar
        staa    SCDR,x          ; transmit data
        pulx
        rts


        include "queue.inc"

isr_sci_error:
        ldab    SCDR,y          ; clear OR/NF/FE
isr_sci:
        ldy     device_base
        brset   SCSR,y, #SCSR_OR_bm|SCSR_NF_bm|SCSR_FE_bm, isr_sci_error
        brclr   SCSR,y, #SCSR_RDRF_bm, isr_sci_return
        ldaa    SCDR,y
        ldx     #rx_queue
        jsr     queue_add
isr_sci_return:
        rti
