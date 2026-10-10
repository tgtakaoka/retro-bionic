        include "mc68hc12.inc"

        org     $1100

rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size

;;; SCI: Enable Rx interrupt
RX_INT_TX_NO:   equ     SC0CR2_TE_bm|SC0CR2_RE_bm|SC0CR2_RIE_bm

stack:  equ     $2000           ; SP points at the last byte pushed

        org     VEC_SCI0
        fdb     isr_sci

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $2000
device_base:
        fdb     $0000
initialize:
        lds     #stack
        ldx     #rx_queue
        ldab    #rx_queue_size
        jsr     queue_init
        ;; Initialize SCI
        ldx     device_base
        clr     SC0CR1,x         ; 8bit 1stop
        clr     SC0BDH,x
        ldaa    #1              ; SBR=1: E/16
        staa    SC0BDL,x
        ldaa    #RX_INT_TX_NO
        staa    SC0CR2,x         ; Enable Tx and Rx/Interrupt
        cli                     ; Enable IRQ
        ldx     #rx_queue
        bra     loop

wait:
        bra     loop            ; spin, IRQ enabled: it comes in running code
loop:
        sei                     ; Disable IRQ
        jsr     queue_remove
        cli                     ; Enable IRQ
        bcc     wait
        tsta
        beq     halt_to_system
        bsr     putchar
        cmpa    #$0D
        bne     loop
        ldaa    #$0A
        bsr     putchar
        bra     loop
halt_to_system:
        swi                     ; halt_to_system

putchar:
        pshx
        ldx     device_base
;;; Transmit Data Register Empty?
        brclr   SC0SR1,x, #SC0SR1_TDRE_bm, putchar
        staa    SC0DRL,x          ; transmit data
        pulx
        rts


        include "../mc6801/queue.inc"

isr_sci_error:
        ldab    SC0DRL,y          ; clear OR/NF/FE
isr_sci:
        ldy     device_base
        brset   SC0SR1,y, #SC0SR1_OR_bm|SC0SR1_NF_bm|SC0SR1_FE_bm, isr_sci_error
        brclr   SC0SR1,y, #SC0SR1_RDRF_bm, isr_sci_return
        ldaa    SC0DRL,y
        ldx     #rx_queue
        jsr     queue_add
isr_sci_return:
        rti
