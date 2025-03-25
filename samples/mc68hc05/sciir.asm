        include "mc68hc05c0.inc"

        org     $0080
rx_queue_size:  equ     16
rx_queue:
        rmb     rx_queue_size

        org     VEC_SCI
        fdb     isr_sci

        org     VEC_SWI
        fdb     VEC_SWI         ; halt to system

        org     VEC_RESET
        fdb     initialize

        org     $1000
initialize:
        ldx     #rx_queue
        lda     #rx_queue_size
        jsr     queue_init
        ;; Initialize SCI
        clr     SCCR1
        lda     #(1<<SCBR_SCP_gp)|(3<<SCBR_SCT_gp)|(3<<SCBR_SCR_gp)
        sta     SCBR            ; prescaler=1/3 Tx/Rx=1/8
        bset    SCCR2_TE_bp,SCCR2  ; Enable Tx
        bset    SCCR2_RE_bp,SCCR2  ; Enable Rx
        bset    SCCR2_RIE_bp,SCCR2 ; Enable Rx interrupt
        bset    EICSR_IRQEL_bp,EICSR ; Negative-edge and level sensitive #IRQ
        bset    EICSR_IRQEN_bp,EICSR ; Enable IRQ
loop:
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
        brclr   SCSR_TDRE_bp,SCSR,putchar
        sta     SCDR
        rts

        include "queue.inc"

isr_sci:
        brclr   SCSR_RDRF_bp,SCSR,isr_sci_exit
        lda     SCDR
        ldx     #rx_queue
        jsr     queue_add
isr_sci_exit:
        rti
