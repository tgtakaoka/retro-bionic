;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Console through the ACIA's interrupt: received characters queue at
;;; rx_queue, characters to send at tx_queue.
rx_queue:       equ     0x2000
rx_queue_size:  equ     128
tx_queue:       equ     0x2080
tx_queue_size:  equ     128
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

;;; Initialize the queues and the ACIA
;;; @clobber R0 R1
init_io:
        movd    rx_queue, r1
        movd    rx_queue_size, r0
        bsr     queue_init
        movd    tx_queue, r1
        movd    tx_queue_size, r0
        bsr     queue_init
        movb    CDS_RESET_gc, @ACIA_control ; Master reset
        movb    RX_INT_TX_NO, @ACIA_control ; disable Tx interrupt
        ret     0

;;; Get character
;;; @return R0
;;; @return PSR.F 0 if no character
getchar:
        save    [r1]
        movd    rx_queue, r1
        bicpsrw PSR_I           ; disable interrupt
        bsr     queue_remove
        bispsrw PSR_I           ; enable interrupt
        restore [r1]
        ret     0

;;; Put newline
;;; @clobber R0
newline:
        movb    0x0D, r0
        bsr     putchar
        movb    0x0A, r0
;;; Put character
;;; @param R0
putchar:
        save    [r1]
        movd    tx_queue, r1
putchar_retry:
        bicpsrw PSR_I           ; disable interrupt
        bsr     queue_add
        bispsrw PSR_I           ; enable interrupt
        bfc     putchar_retry   ; branch if queue is full
        movb    RX_INT_TX_INT, @ACIA_control ; enable Tx interrupt
        restore [r1]
        ret     0

;;; Put space
;;; @clobber R0
putspace:
        movb    ' ', r0
        br      putchar

;;; Wait until the transmit queue is empty
;;; @clobber R0
queue_wait_empty:
        cmpqb   0, @tx_queue+queue_len
        bne     queue_wait_empty
        ret     0

;;; INT, non-vectored: receive or send, then RETT, which pops the PSR
;;; with I set again.
isr_nvi:
        save    [r0,r1,r2]
        movzbd  @ACIA_status, r2
        tbitb   7, r2           ; IRQF
        bfc     isr_nvi_exit
        tbitb   0, r2           ; RDRF
        bfc     isr_nvi_send
        movzbd  @ACIA_data, r0  ; receive character
        movd    rx_queue, r1
        bsr     queue_add
isr_nvi_send:
        tbitb   1, r2           ; TDRE
        bfc     isr_nvi_exit
        movd    tx_queue, r1
        bsr     queue_remove
        bfc     isr_nvi_send_empty
        movb    r0, @ACIA_data  ; send character
isr_nvi_exit:
        restore [r0,r1,r2]
        rett    0
isr_nvi_send_empty:
        movb    RX_INT_TX_NO, @ACIA_control ; disable Tx interrupt
        restore [r0,r1,r2]
        rett    0

        include "queue.inc"
