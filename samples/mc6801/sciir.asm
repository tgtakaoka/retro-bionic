        cpu     6801
        include "mc6801.inc"

        org     $2000

rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size

;;; SCI: Enable Rx interrupt
RX_INT_TX_NO:   equ     TRCSR_RE_bm|TRCSR_RIE_bm|TRCSR_TE_bm

        org     $1000
stack:  equ     *-1             ; MC6801's SP is post-decrement/pre-increment

        org     VEC_SCI
        fdb     isr_sci

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
        ;; Initialize SCI
        ldaa    #CCFS_NRZ_gc|SS_DIV128_gc
        staa    RMCR            ; set NRZ and E/128
        ldaa    #RX_INT_TX_NO
        staa    TRCSR           ; Enable Tx and Rx/Interrupt
        cli                     ; Enable IRQ
        bra     loop

wait:   wai
loop:   bsr     getchar
        bcc     wait
        bsr     putchar
        tsta
        beq     halt_to_system
        cmpa    #$0D
        bne     loop
        ldaa    #$0A
        bsr     putchar
        bra     loop
halt_to_system:
        swi

getchar:
        pshx
        ldx     #rx_queue
        sei                     ; Disable IRQ
        jsr     queue_remove
        cli                     ; Enable IRQ
        pulx
        rts

putchar:
        psha
putchar_loop:
        ldaa    TRCSR
        bita    #TRCSR_TDRE_bm  ; Transmit Data Register Empty?
        beq     putchar_loop    ; no
        pula
        staa    SCTDR           ; Transmit data
        rts

        include "queue.inc"

isr_sci:
        ldab    TRCSR
        bitb    #TRCSR_RDRF_bm
        beq     isr_sci_return
        ldaa    SCRDR
        ldx     #rx_queue
        jsr     queue_add
isr_sci_return:
        rti
