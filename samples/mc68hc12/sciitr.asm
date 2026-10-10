        include "mc68hc12.inc"

        org     $1100

rx_queue_size:  equ     128
rx_queue:       rmb     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       rmb     tx_queue_size
tx_int_control: rmb     1

;;; SCI: Enable Rx interrupt
RX_INT_TX_NO:   equ     SC0CR2_TE_bm|SC0CR2_RE_bm|SC0CR2_RIE_bm
RX_INT_TX_INT:  equ     SC0CR2_TE_bm|SC0CR2_RE_bm|SC0CR2_RIE_bm|SC0CR2_TIE_bm

stack:  equ     $2000           ; SP points at the last byte pushed

        org     VEC_SCI0
        fdb     isr_sci

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $2000
device_base:
        fdb     $0000           ; device base
initialize:
        lds     #stack
        ldx     #rx_queue
        ldab    #rx_queue_size
        jsr     queue_init
        ldx     #tx_queue
        ldab    #tx_queue_size
        jsr     queue_init
        ;; Initialize SCI
        ldy     device_base
        clr     SC0CR1,y                    ; 8bit 1stop
        clr     SC0BDH,y
        ldaa    #1              ; SBR=1: E/16
        staa    SC0BDL,y
        ldaa    #RX_INT_TX_NO
        staa    SC0CR2,y         ; enable Tx and Rx/Interrupt
        clr     tx_int_control  ; disable Tx interrupt
        cli                     ; enable IRQ
        bra     loop

wait:
        wai
loop:
        bsr     getchar
        bcc     wait
        tsta
        beq     halt_to_system
        tab
        bsr     putchar         ; echo
        ldaa    #' '            ; space
        bsr     putchar
        bsr     put_hex8        ; print in hex
        ldaa    #' '            ; space
        bsr     putchar
        bsr     put_bin8        ; print in binary
        bsr     newline
        bra     loop
halt_to_system:
        swi

;;; Put newline
;;; @clobber A
newline:
        ldaa    #$0D
        bsr     putchar
        ldaa    #$0A
        bra     putchar

;;; Print uint8_t in hex
;;; @param B uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        ldaa    #'0'
        bsr     putchar
        ldaa    #'x'
        bsr    putchar
        tba
        lsra
        lsra
        lsra
        lsra
        bsr     put_hex4
        tba
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
        pshb
        ldaa    #'0'
        bsr     putchar
        ldaa    #'b'
        bsr     putchar
        bsr     put_bin4
        bsr     put_bin4
        pulb
        rts
put_bin4:
        bsr     put_bin2
put_bin2:
        bsr     put_bin1
put_bin1:
        ldaa    #'0'
        lslb                    ; C=MSB
        bcc     putchar         ; MSB=0
        inca                    ; MSB=1
        bra     putchar

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
getchar:
        pshx
        sei                     ; disable IRQ
        ldx     #rx_queue
        jsr     queue_remove
        cli                     ; enable IRQ
        pulx
        rts

;;; Put character
;;; @param A
putchar:
        psha
        pshx
        ldx     #tx_queue
putchar_retry:
        sei                     ; disable IRQ
        jsr     queue_add
        cli                     ; enable IRQ
        bcc     putchar_retry   ; branch if queue is full
        ldx     device_base
        ldaa    #RX_INT_TX_INT  ; Enable Tx interrupt
        staa    SC0CR2,x
        pulx
        pula
        rts

        include "../mc6801/queue.inc"

isr_sci:
        ldy     device_base
        brclr   SC0SR1,y, #SC0SR1_OR_bm|SC0SR1_NF_bm|SC0SR1_FE_bm, isr_sci_receive
        ldaa    SC0DRL,y          ; reset ORFE
isr_sci_receive:
        brclr   SC0SR1,y, #SC0SR1_RDRF_bm, isr_sci_send
        ldaa    SC0DRL,y          ; receive character
        ldx     #rx_queue
        jsr     queue_add
isr_sci_send:
        brclr   SC0SR1,y, #SC0SR1_TDRE_bm, isr_sci_exit
        ldx     #tx_queue
        jsr     queue_remove
        bcc     isr_sci_empty
        staa    SC0DRL,y          ; send character
isr_sci_exit:
        rti
isr_sci_empty:
        ldaa    #RX_INT_TX_NO
        staa    SC0CR2,y         ; disable Tx interrupt
        rti
