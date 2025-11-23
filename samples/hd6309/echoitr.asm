        include "hd6309.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "../mc6800/mc6850.inc"

        org     $2000

rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       rmb     tx_queue_size
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     $1000
stack:  equ     *

        org     VEC_FIRQ
        fdb     isr_firq

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $1000
initialize:
        ldmd    #1              ; 6309 native mode
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
        lda     #VEC_FIRQ       ; FIRQ
        sta     ACIA+2          ; set #FIRQ name for MC6850 emulator

wait:   
        cwai    #~CC_FIRQ       ; Clear FIRQ mask
loop:
        bsr     getchar
        bcc     loop
        tsta
        beq     halt_to_system
        tfr     a,b
        bsr     putchar         ; echo
        lda     #' '            ; space
        bsr     putchar
        bsr     put_hex8        ; print in hex
        lda     #' '
        bsr     putchar
        bsr     put_bin8        ; print in binary
        bsr     newline
        bra     loop
halt_to_system:
        swi                     ; halt to system

;;; Put newline
;;; @clobber A
newline:
        lda     #$0D
        bsr     putchar
        lda     #$0A
        bra     putchar

;;; Print uint8_t in hex
;;; @param B uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        lda     #'0'
        bsr     putchar
        lda     #'x'
        bsr     putchar
        tfr     b,a
        lsra
        lsra
        lsra
        lsra
        bsr     put_hex4
        tfr     b,a
put_hex4:
        anda    #$0f
        cmpa    #10
        blo     put_hex8_dec
        adda    #'A'-10
        bra     putchar
put_hex8_dec:
        adda    #'0'
        bra     putchar

;;; Print uint8_t in binary
;;; @param B uint8_t value to be printed in binary.
;;; @clobber A
put_bin8:
        pshs    b
        lda     #'0'
        bsr     putchar
        lda     #'b'
        bsr     putchar
        bsr     put_bin4
        bsr     put_bin4
        puls    b,pc
put_bin4:
        bsr     put_bin2
put_bin2:
        bsr     put_bin1
put_bin1:
        lda     #'0'
        lslb                    ; C=MSB
        bcc     putchar         ; MSB=0
        inca                    ; MSB=1
        bra     putchar

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
getchar:
        pshs    x
        ldx     #rx_queue
        orcc    #CC_FIRQ         ; disable FIRQ
        bsr     queue_remove
        andcc   #~CC_FIRQ        ; enable FIRQ
        puls    x,pc

;;; Put character
;;; @param A
putchar:
        pshs    x,a
        ldx     #tx_queue
putchar_retry:
        orcc    #CC_FIRQ         ; disable FIRQ
        bsr     queue_add
        andcc   #~CC_FIRQ        ; enable FIRQ
        bcc     putchar_retry    ; branch if queue is full
        lda     #RX_INT_TX_INT   ; enable Tx interrupt
        sta     ACIA_control
        puls    a,x,pc

        include "../mc6809/queue.inc"

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
