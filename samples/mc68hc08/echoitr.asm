        include "mc68hc08az0.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FFE0
        include "../mc6800/mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     $2000
rx_queue_size:  equ     128
rx_queue:
        rmb     rx_queue_size
tx_queue_size:  equ     128
tx_queue:
        rmb     tx_queue_size

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
        ldhx    #tx_queue
        lda     #tx_queue_size
        jsr     queue_init
        ;; initialize ACIA
        lda     #CDS_RESET_gc   ; master reset
        sta     ACIA_control
        lda     #RX_INT_TX_NO
        sta     ACIA_control
        cli                     ; enable IRQ
loop:
        lda     COP_RESET
        sta     COP_RESET
        bsr     getchar
        bcc     loop
        tsta
        beq     halt_to_system
        bsr     putchar         ; echo
        psha
        lda     #' '            ; space
        bsr     putchar
        lda     1,sp
        bsr     put_hex8        ; print in hex
        lda     #' '            ; space
        bsr     putchar
        pula
        bsr     put_bin8        ; print in binary
        bsr     newline
        bra     loop
halt_to_system:
        swi

;;; Put newline
;;; @clobber A
newline:
        lda     #$0D
        bsr     putchar
        lda     #$0A
        bra     putchar

;;; Print uint8_t in hex
;;; @param A uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        psha
        lda     #'0'
        bsr     putchar
        lda     #'x'
        bsr     putchar
        lda     1,sp
        lsra
        lsra
        lsra
        lsra
        bsr     put_hex4
        pula
put_hex4:
        and     #$0F
        add     #$90
        daa
        adc     #$40
        daa
        bra     putchar

;;; Print uint8_t in binary
;;; @param A uint8_t value to be printed in binary.
;;; @clobber A
put_bin8:
        psha
        lda     #'0'
        bsr     putchar
        lda     #'b'
        bsr     putchar
        pula
        bsr     put_bin4
put_bin4:
        bsr     put_bin2
put_bin2:
        bsr     put_bin1
put_bin1:
        psha
        lda     #'0'
        rol     1,sp
        bcc     put_bin
        inca
put_bin:
        bsr     putchar
        pula
        rts

;;; Get character
;;; @clobber HX
;;; @return A
;;; @return CC.C 0 if no char received
getchar:
        ldhx    #rx_queue
        sei                     ; disable IRQ
        jsr     queue_remove
        cli                     ; enable IRQ
        rts

;;; Put character
;;; @param A
;;; @clobber A HX
putchar:
        ldhx    #tx_queue
        psha
putchar_retry:
        sei                     ; disable IRQ
        jsr     queue_add
        cli                     ; enable IRQ
        bcc     putchar_retry   ; branch if queue is full
        lda     #RX_INT_TX_INT  ; enable Tx interrupt
        sta     ACIA_control
putchar_exit:
        pula
        rts

        include "queue.inc"

isr_irq:
        pshh
        lda     ACIA_status
        bit     #IRQF_bm
        beq     isr_irq_exit
        lda     ACIA_status
        bit     #RDRF_bm
        beq     isr_irq_send
        lda     ACIA_data       ; receive character
        ldhx    #rx_queue
        jsr     queue_add
isr_irq_send:
        lda     ACIA_status
        bit     #TDRE_bm
        beq     isr_irq_exit
        ldhx    #tx_queue
        jsr     queue_remove
        bcc     isr_irq_send_empty
        sta     ACIA_data       ; send character
isr_irq_exit:
        pulh
        rti
isr_irq_send_empty:
        lda     #RX_INT_TX_NO
        sta     ACIA_control    ; disable Tx interrupt
        pulh
        rti
