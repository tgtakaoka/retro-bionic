        include "hd641016.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FFDF00
        include "../mc6800/mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     $2000

rx_queue_size:  equ     128
rx_queue:       .res.b  rx_queue_size
tx_queue_size:  equ     128
tx_queue:       .res.b  tx_queue_size

stack:  equ     $1000

        org     VEC_IRQ0
        .data.l isr_irq

        org     VEC_TRAPA15
        .data.l VEC_TRAPA15     ; for halt to system

        org     VEC_RESET
        .data.l stack
        .data.l initialize

        org     $1000
initialize:
        mov.l   #rx_queue, r1
        mov.w   #rx_queue_size, r0
        bsr     queue_init
        mov.l   #tx_queue, r1
        mov.w   #tx_queue_size, r0
        bsr     queue_init
        ;; initialize ACIA
        mov.b   #CDS_RESET_gc, @ACIA_control ; master reset
        mov.b   #RX_INT_TX_NO, @ACIA_control ; disable Tx interrupt
        ;; #IRQ0 autovectored and low-level triggered (ICR at reset), level 1
        mov.w   #IR0P_LOW_gc, @IPR2
        andc    #~$0700, sr     ; enable IRQ
        bra     loop

wait:
        sleep                   ; an interrupt wakes it
loop:
        bsr     getchar
        bcc     wait
        tst.b   r0
        beq     halt_to_system
        mov.b   r0, r3
        bsr     putchar         ; echo
        mov.b   #' ', r0        ; space
        bsr     putchar
        bsr     put_hex8        ; print in hex
        mov.b   #' ', r0        ; space
        bsr     putchar
        bsr     put_bin8        ; print in binary
        bsr     newline
        bra     loop
halt_to_system:
        trapa   #15             ; halt to system

;;; Put newline
;;; @clobber R0
newline:
        mov.b   #$0D, r0
        bsr     putchar
        mov.b   #$0A, r0
        bra     putchar

;;; Print uint8_t in hex
;;; @param R3 uint8_t value to be printed in hex.
;;; @clobber R0
put_hex8:
        mov.b   #'0', r0
        bsr     putchar
        mov.b   #'x', r0
        bsr     putchar
        mov.b   r3, r0
        shlr.b  #4, r0
        bsr     put_hex4
        mov.b   r3, r0
put_hex4:
        and.b   #$0F, r0
        cmp.b   #10, r0
        blo     put_hex8_dec
        add.b   #'A'-10, r0
        bra     putchar
put_hex8_dec:
        add.b   #'0', r0
        bra     putchar

;;; Print uint8_t in binary
;;; @param R3 uint8_t value to be printed in binary.
;;; @clobber R0
put_bin8:
        stm.l   r3, @-r15
        mov.b   #'0', r0
        bsr     putchar
        mov.b   #'b', r0
        bsr     putchar
        bsr     put_bin4
        bsr     put_bin4
        ldm.l   @r15+, r3
        rts
put_bin4:
        bsr     put_bin2
put_bin2:
        bsr     put_bin1
put_bin1:
        mov.b   #'0', r0
        shll.b  #1, r3          ; C=MSB
        bcc     putchar         ; MSB=0
        add.b   #1, r0          ; MSB=1
        bra     putchar

;;; Get character
;;; @return R0
;;; @return CCR.C 0 if no character
getchar:
        stm.l   r1, @-r15
        orc     #$0700, sr      ; disable IRQ
        mov.l   #rx_queue, r1
        bsr     queue_remove
        andc    #~$0700, sr     ; enable IRQ
        ldm.l   @r15+, r1
        rts

;;; Put character
;;; @param R0
putchar:
        stm.l   r0-r1, @-r15
        mov.l   #tx_queue, r1
putchar_retry:
        orc     #$0700, sr      ; disable IRQ
        bsr     queue_add
        andc    #~$0700, sr     ; enable IRQ
        bcc     putchar_retry   ; branch if queue is full
        mov.b   #RX_INT_TX_INT, @ACIA_control ; enable Tx interrupt
        ldm.l   @r15+, r0-r1
        rts

        include "queue.inc"

isr_irq:
        stm.l   r0-r2, @-r15    ; an interrupt stacks only PC and SR
        mov.b   @ACIA_status, r2
        btst.b  #7, r2          ; IRQF
        beq     isr_irq_exit
        btst.b  #0, r2          ; RDRF
        beq     isr_irq_send
        mov.b   @ACIA_data, r0  ; receive character
        mov.l   #rx_queue, r1
        bsr     queue_add
isr_irq_send:
        btst.b  #1, r2          ; TDRE
        beq     isr_irq_exit
        mov.l   #tx_queue, r1
        bsr     queue_remove
        bcc     isr_irq_send_empty
        mov.b   r0, @ACIA_data  ; send character
isr_irq_exit:
        ldm.l   @r15+, r0-r2
        rte
isr_irq_send_empty:
        mov.b   #RX_INT_TX_NO, @ACIA_control ; disable Tx interrupt
        ldm.l   @r15+, r0-r2
        rte
