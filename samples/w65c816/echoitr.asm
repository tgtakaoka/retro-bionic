;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     65816
        .include "w65c816.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       $DF00
        .include "mc6850.inc"
RX_INT_TX_NO    =       WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT   =       WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        *=      NVEC_IRQ
        .word   isr_irq

        *=      VEC_RESET
        .word   initialize

        *=      $2000
rx_queue_size   =       128
rx_queue:
        *=      *+rx_queue_size
tx_queue_size   =       128
tx_queue:
        *=      *+tx_queue_size

        *=      $1000
stack   =       $01ff
initialize:
        clc
        xce                     ; native mode
        rep     #P_X            ; 16-bit index
        longi   on
        ldx     #stack
        txs
        cld                     ; clear decimal flag
        ldx     #rx_queue
        lda     #rx_queue_size
        jsr     queue_init
        ldx     #tx_queue
        lda     #tx_queue_size
        jsr     queue_init
        ;; initialize ACIA
        lda     #CDS_RESET_gc   ; master reset
        sta     ACIA_control
        lda     #RX_INT_TX_NO
        sta     ACIA_control
        cli                     ; enable IRQ

receive_loop:
        jsr     getchar
        bcs     receive_loop
        ora     #0
        beq     halt_to_system
echo_back:
        pha
        jsr     putchar         ; echo
        lda     #' '            ; space
        jsr     putchar
        lda     1,S
        jsr     put_hex8        ; print in hex
        lda     #' '            ; space
        jsr     putchar
        pla
        jsr     put_bin8        ; print in binary
        jsr     newline
        bra     receive_loop
halt_to_system:
        brk
        .byte    0              ; halt to system

;;; Put newline
;;; @clobber A
newline:
        lda     #$0D
        jsr     putchar
        lda     #$0A
        bra     putchar

;;; Print uint8_t in hex
;;; @param A uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        pha
        lda     #'0'
        jsr     putchar
        lda     #'x'
        jsr    putchar
        lda     1,S
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jsr     put_hex4
        pla
put_hex4:
        and     #$0F
        cmp     #10
        bcc     put_hex8_dec    ; <10
        adc     #'A'-10-1       ; C=1
        bra     putchar
put_hex8_dec:
        adc     #'0'
        bra     putchar

;;; Print uint8_t in binary
;;; @param A uint8_t value to be printed in binary.
;;; @clobber A
put_bin8:
        pha
        lda     #'0'
        jsr     putchar
        lda     #'b'
        jsr     putchar
        pla
        jsr     put_bin4
put_bin4:
        jsr     put_bin2
put_bin2:
        jsr     put_bin1
put_bin1:
        asl     a               ; C=MSB
        pha
        bcc     put_bin0
        lda     #'1'
        bra     put_binchar
put_bin0:
        lda     #'0'
put_binchar:
        jsr     putchar
        pla
        rts

;;; Get character
;;; @return A
;;; @return P.C 1 if no character
;;; @clobber X
getchar:
        ldx     #rx_queue
        sei                     ; disable IRQ
        jsr     queue_remove
        cli
        rts

;;; Put character
;;; @param A
;;; @clobber X
putchar:
        pha
putchar_retry:
        ldx     #tx_queue
        sei                     ; disable IRQ
        jsr     queue_add
        cli                     ; enable IRQ
        bcs     putchar_retry   ; queue is full
        lda     #RX_INT_TX_INT  ; enable Tx interrupt
        sta     ACIA_control
        pla
        rts

        .include "queue.inc"

isr_irq:
        ;; P_D is cleared on interrupt
        sep     #P_M            ; 8-bit memory
        pha                     ; save A
        phx                     ; save X
        lda     ACIA_status
        and     #IRQF_bm
        beq     isr_irq_exit
        lda     ACIA_status
        and     #RDRF_bm
        beq     isr_irq_send
        lda     ACIA_data       ; receive character
        ldx     #rx_queue
        jsr     queue_add
isr_irq_send:
        lda     ACIA_status
        and     #TDRE_bm
        beq     isr_irq_exit
        ldx     #tx_queue
        jsr     queue_remove
        bcs     isr_irq_send_empty
        sta     ACIA_data       ; send character
isr_irq_exit:
        plx                     ; restore X
        pla                     ; restore Y
        rti                     ; restore P and PC
isr_irq_send_empty:
        lda     #RX_INT_TX_NO
        sta     ACIA_control    ; disable Tx interrupt
        bra     isr_irq_exit
