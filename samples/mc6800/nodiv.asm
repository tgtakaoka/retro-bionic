        cpu     6800
        include "mc6800.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     $40
;;; 0.0458 * 50 = 2.29
;;;   2.29 * 100 = 229
;;; 0.0833 * 50 = 4.165
;;;   4.1665 * 100 = 416.5
;;; 0.0458 * 64 = 2.9312
;;;   2.9312 * 128 = 375.1936
;;; 0.08333 * 64 = 5.3312
;;;   5.3312 * 128 = 682.3936
;;; Working space for mandelbrot.inc
KA:     equ     375
KB:     equ     682
S:      equ     128
F:      equ     64
vC:     rmb     2
vD:     rmb     2
vA:     rmb     2
vB:     rmb     2
vS:     rmb     2
vP:     rmb     2
vQ:     rmb     2
vT:     rmb     2
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
sign:   rmb     1

        org     $2000
rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       rmb     tx_queue_size

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
        ldx     #tx_queue
        ldab    #tx_queue_size
        jsr     queue_init
        ;; initialize ACIA
        ldaa    #CDS_RESET_gc   ; master reset
        staa    ACIA_control
        ldaa    #RX_INT_TX_NO
        staa    ACIA_control
        cli                     ; enable IRQ

loop:
        jsr     mandelbrot
        jsr     newline
        bra     loop

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
;;; @clobber X
getchar:
        sei                     ; disable IRQ
        ldx     #rx_queue
        jsr     queue_remove
        cli                     ; enable IRQ
        rts

;;; Put character
;;; @param A
;;; @clobber R2
putspace:
        ldaa    #' '
        bra     putchar
newline:
        ldaa    #$0D
        bsr     putchar
        ldaa    #$0A
putchar:
        stx     R2
        psha
        ldx     #tx_queue
putchar_retry:
        sei                     ; disable IRQ
        jsr     queue_add
        cli                     ; enable IRQ
        bcc     putchar_retry   ; branch if queue is full
        ldaa    #RX_INT_TX_INT  ; enable Tx interrupt
        staa    ACIA_control
        pula
        ldx     R2
        rts

        include "nodiv.inc"
        include "arith.inc"
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
