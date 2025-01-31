        cpu     6801
        include "mc6801.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     $2000

rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       rmb     tx_queue_size

        org     $1000
stack:  equ     *-1             ; MC6801's SP is post-decrement/pre-increment

        org     $FFF2           ; MC68HC11 IRQ
        fdb     isr_irq

        org     $FFF6           ; MC68HC11 SWI
        fdb     $FFF6

        org     VEC_IRQ1
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
        ldx     #tx_queue
        ldab    #tx_queue_size
        jsr     queue_init
        ;; initialize ACIA
        ldaa    #CDS_RESET_gc   ; master reset
        staa    ACIA_control
        ldaa    #RX_INT_TX_NO   ; disable Tx interrupt
        staa    ACIA_control
        cli                     ; enable IRQ
        bra     loop

wait:
        wai
loop:
        bsr     getchar
        bcc     wait
        tsta
        beq     halt_to_system
        tab
        bsr     putchar         ; echo
        ldaa    #' '            ; space
        bsr     putchar
        bsr     put_hex8        ; print in hex
        ldaa    #' '            ; space
        bsr     putchar
        bsr     put_bin8        ; print in binary
        bsr     newline
        bra     loop
halt_to_system:
        swi

;;; Put newline
;;; @clobber A
newline:
        ldaa    #$0D
        bsr     putchar
        ldaa    #$0A
        bra     putchar

;;; Print uint8_t in hex
;;; @param B uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        ldaa    #'0'
        bsr     putchar
        ldaa    #'x'
        bsr     putchar
        tba
        lsra
        lsra
        lsra
        lsra
        bsr     put_hex4
        tba
put_hex4:
        anda    #$0F
        adda    #$90            ; $90-$9F
        daa                     ; $90-$09, $00-$05(C=1)
        adca    #$40            ; $D0-$D9, $41-$46
        daa                     ; $30-$39, $41-$46
        bra     putchar

;;; Print uint8_t in binary
;;; @param B uint8_t value to be printed in binary.
;;; @clobber A
put_bin8:
        pshb
        ldaa    #'0'
        bsr     putchar
        ldaa    #'b'
        bsr     putchar
        bsr     put_bin4
        bsr     put_bin4
        pulb
        rts
put_bin4:
        bsr     put_bin2
put_bin2:
        bsr     put_bin1
put_bin1:
        ldaa    #'0'
        lslb                    ; C=MSB
        bcc     putchar         ; MSB=0
        inca                    ; MSB=1
        bra     putchar

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
getchar:
        pshx
        sei                     ; disable IRQ
        ldx     #rx_queue
        jsr     queue_remove
        cli
        pulx
        rts

;;; Put character
;;; @param A
putchar:
        psha
        pshx
        ldx     #tx_queue
putchar_retry:
        sei                     ; disable IRQ
        jsr     queue_add
        cli                     ; enable IRQ
        bcc     putchar_retry   ; branch if queue is full
        ldaa    #RX_INT_TX_INT  ; enable Tx interrupt
        staa    ACIA_control
        pulx
        pula
        rts

        include "queue.inc"

isr_irq:
        ldab    ACIA_status
        bitb    #IRQF_bm
        beq     isr_irq_exit
        bitb    #RDRF_bm
        beq     isr_irq_send
        ldaa    ACIA_data       ; receive character
        ldx     #rx_queue
        jsr     queue_add
isr_irq_send:
        bitb    #TDRE_bm
        beq     isr_irq_exit
        ldx     #tx_queue
        jsr     queue_remove
        bcc     isr_irq_send_empty
        staa    ACIA_data       ; send character
isr_irq_exit:
        rti
isr_irq_send_empty:
        ldaa    #RX_INT_TX_NO
        staa    ACIA_control    ; disable Tx interrupt
        rti
