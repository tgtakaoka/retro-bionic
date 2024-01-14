;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     65816
        .include "w65c816.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       $DF00
        .include "mc6850.inc"
RX_INT_TX_NO    =       WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT   =       WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        *=      NVEC_IRQ        ; native vector
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

;;; Work area for mandelbrot.inc
        *=      $10
F       =       50
vY:     .word   0
vX:     .word   0
vI:     .word   0
vA:     .word   0
vB:     .word   0
vC:     .word   0
vD:     .word   0
vP:     .word   0
vQ:     .word   0
vS:     .word   0
vT:     .word   0

        *=      $1000
stack   =       *-1
initialize:
        clc
        xce                     ; native mode
        longa   off
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

        jsr     mandelbrot
        jsr     newline
        sep     #P_M            ; 8-bit memory
wait:   lda     tx_queue        ; tx queue len
        bne     wait
        brk
        .byte   0               ; halt to system

;;; Get character
;;; @return A
;;; @return P.C 1 if no character
;;; @clobber X
getchar:
        sep     #P_M            ; 8-bit memory
        longa   off
        ldx     #rx_queue
        sei                     ; disable IRQ
        jsr     queue_remove
        rep     #P_M|P_IRQ      ; enable IRQ, 16-bit memory
        rts

;;; Put character
;;; @param A
;;; @clobber X
putchar:
        php
        sep     #P_M            ; 8-bit memory
        longa   off
        jsr     _putchar
        plp
        rts
_putchar:       
        pha
        phx
_putchar_retry:
        ldx     #tx_queue
        sei                     ; disable IRQ
        jsr     queue_add
        cli                     ; enable IRQ
        bcs     _putchar_retry  ; queue is full
        lda     #RX_INT_TX_INT  ; enable Tx interrupt
        sta     ACIA_control
        plx
        pla
        rts

;;; Put newline
;;; @clobber A
newline:
        php
        sep     #P_M            ; 8-bit memory
        longa   off
        lda     #$0D
        jsr     _putchar
        lda     #$0A
        jsr     _putchar
        plp
        rts

;;; Put space
;;; @clobber A
putspace:
        php
        sep     #P_M            ; 8-bit memory
        longa   off
        lda     #' '
        jsr     _putchar
        plp
        rts

        .include "queue.inc"
        .include "arith.inc"
        .include "mandelbrot.inc"

isr_irq:
        ;; P_D is cleared on interrupt
        sep     #P_M            ; 8-bit memory
        longa   off
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
