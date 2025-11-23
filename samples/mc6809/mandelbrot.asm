        include "mc6809.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "../mc6800/mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     $20
;;; Working space for mandelbrot.inc
F:      equ     50
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

        org     $2000
rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       rmb     tx_queue_size

        org     $1000
stack:  equ     *-1             ; MC6800's SP is post-decrement/pre-increment

        org     VEC_FIRQ
        fdb     isr_firq

        org     VEC_IRQ
        fdb     isr_irq

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_NMI
        fdb     isr_nmi

        org     VEC_RESET
        fdb     main

        org     $0100
main:
        lds     #stack
        ldx     #rx_queue
        ldb     #rx_queue_size
        lbsr    queue_init
        ldx     #tx_queue
        ldb     #tx_queue_size
        lbsr    queue_init
        ;; initialize ACIA
        lda     #CDS_RESET_gc   ; master reset
        sta     ACIA_control
        lda     #RX_INT_TX_NO
        sta     ACIA_control
        lda     #VEC_FIRQ
        sta     ACIA+2          ; set #FIRQ name for MC6850 emulator
        andcc   #~CC_FIRQ       ; Clear FIRQ mask

loop:
        jsr     mandelbrot
        jsr     newline
        bra     loop

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
getchar:
        pshs    x
        ldx     #rx_queue
        orcc    #CC_FIRQ         ; disable FIRQ
        lbsr     queue_remove
        andcc   #~CC_FIRQ        ; enable FIRQ
        puls    x,pc

;;; Put character
;;; @param A
putspace:
        lda     #' '
        bra     putchar
newline:
        lda     #$0D
        bsr     putchar
        lda     #$0A
putchar:
        pshs    x,a
        ldx     #tx_queue
putchar_retry:
        orcc    #CC_FIRQ         ; disable FIRQ
        lbsr    queue_add
        andcc   #~CC_FIRQ        ; enable FIRQ
        bcc     putchar_retry    ; branch if queue is full
        lda     #RX_INT_TX_INT   ; enable Tx interrupt
        sta     ACIA_control
        puls    a,x,pc

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

isr_firq:
        pshs    x,b,a
        ldb     ACIA_status
        bitb    #IRQF_bm
        beq     isr_firq_exit
        bitb    #RDRF_bm
        beq     isr_firq_send
        lda     ACIA_data       ; receive character
        ldx     #rx_queue
        jsr     queue_add
isr_firq_send:
        bitb    #TDRE_bm
        beq     isr_firq_exit
        ldx     #tx_queue
        jsr     queue_remove
        bcc     isr_firq_send_empty
        sta     ACIA_data       ; send character
isr_firq_exit:
        puls    a,b,x
        rti
isr_firq_send_empty:
        lda     #RX_INT_TX_NO
        sta     ACIA_control    ; disable Tx interrupt
        puls    a,b,x
        rti

isr_irq:
        swi

isr_nmi:
        swi
