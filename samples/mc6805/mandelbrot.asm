        include "mc146805e.inc"
        cpu     6805

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $17F8
        include "mc6850.inc"

rx_queue_size:  equ     16
tx_queue_size:  equ     32
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     RAM_START
save_a: rmb     1
save_x: rmb     1
rx_queue:
        rmb     rx_queue_size
tx_queue:
        rmb     tx_queue_size

;;; Working space for mandelbrot.inc
F:      equ     50
vC:     rmb     2
vD:     rmb     2
vA:     rmb     2
vB:     rmb     2
vS:     rmb     2
vP:     rmb     2
vQ:     rmb     2
vY:     rmb     1
vX:     rmb     1
vI:     rmb     1

;;; Working space for arith.inc
R0:
R0H:    rmb     1
R0L:    rmb     1
R1:
R1H:    rmb     1
R1L:    rmb     1
R2:
R2H:    rmb     1
R2L:    rmb     1
arith_work:
        rmb     1
SP:     rmb     1
        org     $0100
stack:  rmb     200

        org     VEC_IRQ
        fdb     isr_irq

        org     VEC_SWI
        fdb     VEC_SWI         ; halt to system

        org     VEC_RESET
        fdb     initialize

        org     $0200
initialize:
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

        clr     SP
loop:
        jsr     mandelbrot
        jsr     newline
        bra     loop

;;; Get character
;;; @clobber X
;;; @return A
;;; @return CC.C 0 if no char received
getchar:
        ldx     #rx_queue
        sei                     ; disable IRQ
        jsr     queue_remove
        cli                     ; enable IRQ
        rts

;;; Put character
;;; @param A
;;; @clobber A
putspace:
        lda     #' '
        bra     putchar
newline:
        lda     #$0D
        bsr     putchar
        lda     #$0A
putchar:
        stx     save_x          ; save X
        ldx     #tx_queue
putchar_retry:
        sei                     ; disable IRQ
        jsr     queue_add
        cli                     ; enable IRQ
        bcc     putchar_retry   ; branch if queue is full
        lda     #RX_INT_TX_INT  ; enable Tx interrupt
        sta     ACIA_control
putchar_exit:
        ldx     save_x          ; restore X
        rts

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

isr_irq:
        lda     ACIA_status
        bit     #IRQF_bm
        beq     isr_irq_exit
        lda     ACIA_status
        bit     #RDRF_bm
        beq     isr_irq_send
        lda     ACIA_data       ; receive character
        ldx     #rx_queue
        jsr     queue_add
isr_irq_send:
        lda     ACIA_status
        bit     #TDRE_bm
        beq     isr_irq_exit
        ldx     #tx_queue
        jsr     queue_remove
        bcc     isr_irq_send_empty
        sta     ACIA_data       ; send character
isr_irq_exit:
        rti
isr_irq_send_empty:
        lda     #RX_INT_TX_NO
        sta     ACIA_control    ; disable Tx interrupt
        rti
