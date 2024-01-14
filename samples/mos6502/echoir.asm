;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6502
        .include "mos6502.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       $DF00
        .include "mc6850.inc"
RX_INT_TX_NO    =     WSB_8N1_gc|RIEB_bm

        *=      VEC_IRQ
        .word   isr_irq

        *=      VEC_RESET
        .word   initialize

        *=      0
queue_tmp:
        *=      *+2
rx_queue:
        *=      *+queue_buf
        .word   $2000
rx_queue_size   =     128

        *=      $1000
stack   =       $01ff
initialize:
        ldx     #stack & $FF
        txs
        cld                     ; clear decimal flag
        ldx     #rx_queue
        lda     #rx_queue_size
        jsr     queue_init
        ;; initialize ACIA
        lda     #CDS_RESET_gc   ; Master reset
        sta     ACIA_control
        lda     #RX_INT_TX_NO
        sta     ACIA_control
        cli                     ; Enable IRQ

loop:
        jsr     getchar
        bcs     loop
        ora     #0
        beq     halt_to_system
        jsr     putchar
        cmp     #$0D
        bne     loop
        lda     #$0A
        jsr     putchar
        jmp     loop
halt_to_system:
        brk
        .byte   0               ; halt to system

getchar:
        ldx     #rx_queue
        sei                     ; Disable IRQ
        jsr     queue_remove
        cli                     ; Enable IRQ
        rts

putchar:
        pha
putchar_loop:
        lda     ACIA_status
        and     #TDRE_bm
        beq     putchar_loop
        pla
        sta     ACIA_data
        rts

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
        beq     isr_irq_return
isr_irq_receive:
        lda     ACIA_status
        and     #RDRF_bm
        beq     isr_irq_return
        lda     ACIA_data
        ldx     #rx_queue
        jsr     queue_add
isr_irq_return:
        pla                     ; restore Y
        tay
        pla                     ; restore X
        tax
        pla                     ; restore A
        rti
