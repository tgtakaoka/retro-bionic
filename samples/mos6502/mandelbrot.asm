;;; -*- mode: asm; mode: flyspell-prog; -*-
        .include "mos6502.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       $DF00
        .include "mc6850.inc"
RX_INT_TX_NO    =       WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT   =       WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        *=      VEC_NMI
        .word   isr_nmi

        *=      VEC_IRQ
        .word   isr_irq

        *=      VEC_RESET
        .word   initialize

        *=      $80
queue_tmp:
        *=      *+2
rx_queue:
        *=      *+queue_buf
        .word   $2000
rx_queue_size   =       128
tx_queue:
        *=      *+queue_buf
        .word   $2000 + rx_queue_size
tx_queue_size   =       128

;;; Work area for arith.inc
        *=      $10
R0:
R0L:    .byte   0
R0H:    .byte   0
R1:
R1L:    .byte   0
R1H:    .byte   0
R2:
R2L:    .byte   0
R2H:    .byte   0
arith_work:
        .word   0
;;; Work area for mandelbrot.inc
F       =       50
vY:     .byte   0
vX:     .byte   0
vC:     .word   0
vD:     .word   0
vA:     .word   0
vB:     .word   0
vS:     .word   0
vP:     .word   0
vQ:     .word   0
vT:     .word   0
vI:     .byte   0

        *=      $1000
stack   =       $01FF
initialize:
        ldx     #stack & $FF
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

loop:
        jsr     mandelbrot
        jsr     newline
        jmp     loop

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

;;; Put newline
;;; @clobber A
newline:
        lda     #$0D
        jsr     putchar
        lda     #$0A
        jmp     putchar

;;; Put space
;;; @clobber A
putspace:
        lda     #' '
        jmp     putchar

        .include "mandelbrot.inc"
        .include "arith.inc"
        .include "queue.inc"

isr_irq:
        cld                     ; clear decimal flag
        pha                     ; save A
        txa
        pha                     ; save X
        tya
        pha                     ; save Y
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
        pla                     ; restore Y
        tay
        pla                     ; restore X
        tax
        pla                     ; restore Y
        rti                     ; restore P and PC
isr_irq_send_empty:
        lda     #RX_INT_TX_NO
        sta     ACIA_control    ; disable Tx interrupt
        bne     isr_irq_exit    ; always branch

isr_nmi:
        brk
        .byte   0
