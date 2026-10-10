        include "mc68hc16.inc"

        include "../mc6800/mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     $1000
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
sign:   rmb     1

        org     $2000
rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       rmb     tx_queue_size

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
        ldx     #tx_queue
        ldab    #tx_queue_size
        jsr     queue_init
        ;; initialize ACIA
        ldaa    #CDS_RESET_gc   ; master reset
        staa    ACIA_control,z
        ldaa    #RX_INT_TX_NO
        staa    ACIA_control,z
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
putspace:
        ldaa    #' '
        bra     putchar
newline:
        ldaa    #$0D
        bsr     putchar
        ldaa    #$0A
putchar:
        pshx
        psha
        ldx     #tx_queue
putchar_retry:
        sei                     ; disable IRQ
        jsr     queue_add
        cli                     ; enable IRQ
        bcc     putchar_retry   ; branch if queue is full
        ldaa    #RX_INT_TX_INT  ; enable Tx interrupt
        staa    ACIA_control,z
        pula
        pulx
        rts

        include "mandelbrot.inc"
        include "arith.inc"
        include "../mc6801/queue.inc"

isr_irq:
        ldab    ACIA_status,z
        bitb    #IRQF_bm
        beq     isr_irq_exit
        bitb    #RDRF_bm
        beq     isr_irq_send
        ldaa    ACIA_data,z     ; receive character
        ldx     #rx_queue
        jsr     queue_add
isr_irq_send:
        bitb    #TDRE_bm
        beq     isr_irq_exit
        ldx     #tx_queue
        jsr     queue_remove
        bcc     isr_irq_send_empty
        staa    ACIA_data,z     ; send character
isr_irq_exit:
        rti
isr_irq_send_empty:
        ldaa    #RX_INT_TX_NO
        staa    ACIA_control,z  ; disable Tx interrupt
        rti
