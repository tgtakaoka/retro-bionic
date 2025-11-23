;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     1804A
        option  "smart-branch", "on"
        include "cdp1802.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     4
        include "../cdp1802/mc6850.inc"

        org     ORG_RESET
        dis                     ; disable interrupt
        dc      X'00'           ; X:P=0:0
        br      scrt_init
        include "scrt.inc"

        org     X'2000'
rx_queue_size:  equ     128
tx_queue_size:  equ     128
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

rx_queue:
        org     *+rx_queue_size
tx_queue:
        org     *+tx_queue_size

stack:  equ     X'1000'-1

        org     X'0100'
main:
        scal    R4, queue_init
        dc      A(rx_queue)
        dc      rx_queue_size
        scal    R4, queue_init
        dc      A(tx_queue)
        dc      tx_queue_size
        ;; initialize ACIA
        rldi    R8, ACIA_config
        sex     R8              ; R8 for out
        out     ACIA_control    ; Master reset
        out     ACIA_control    ; Set mode
        sex     R3
        ret
        dc      X'33'           ; enable interrupt
        sex     R2
        br      loop

ACIA_config:
        dc      CDS_RESET_gc    ; Master reset
        dc      RX_INT_TX_NO

loop:
        scal    R4, getchar
        bz      loop
        glo     R7
        bnz     echo_back
        idl
echo_back:
        scal    R4, putchar
        ldi     ' '
        scal    R4, putchar
        glo     R7
        scal    R4, put_hex8
        ldi     ' '
        scal    R4, putchar
        glo     R7
        scal    R4, put_bin8
        ldi     x'0d'
        scal    R4, putchar
        ldi     x'0a'
        scal    R4, putchar
        br      loop

;;; Print uint8_t in hex
;;; @param D uint8_t value to be printed in binary.
put_hex8:
        plo     R15             ; save D to scratch pad
        glo     R7              ; save R7.0
        stxd
        glo     R15             ; restore D
        ;;
        plo     R7              ; R7.0=data
        ldi     '0'
        scal    R4, putchar
        ldi     'x'
        scal    R4, putchar
        glo     R7
        shr
        shr
        shr
        shr
        scal    R4, put_hex4
        glo     R7
        scal    R4, put_hex4
        ;;
        irx
        ldx                     ; restore R7.0
        plo     R7
        sret    R4

;;; Print hexadecimal digit
;;; @param D nibble
;;; @clobber D
put_hex4:
        ani     x'0F'
        dadi    x'90'
        daci    x'40'
        scal    R4, putchar
        sret    R4

;;; Print uint8_t in binary
;;; @param D uint8_t value to be printed in binary.
;;; @clobber D
put_bin8:
        plo     R15             ; save D to scratch pad
        glo     R7              ; save R7.0
        stxd
        glo     R15             ; restore D
        plo     R7              ; R7.0=data
        ;;
        ldi     '0'
        scal    R4, putchar
        ldi     'b'
        scal    R4, putchar
        ;;
        scal    R4, put_bin4
        scal    R4, put_bin4
        ;;
        irx
        ldx
        plo     R7              ; restore R7
        sret    R4
put_bin4:
        scal    R4, put_bin2
put_bin2:
        scal    R4, put_bin1
put_bin1:
        glo     R7
        shlc                    ; DF=MSB
        plo     R7
        ldi     '0'
        bnf     putchar         ; MSB=0
        ldi     '1'
        br      putchar

;;; Get character
;;; @return R7.0 char
;;; @return A 0 if no char received
getchar:
        sex     R3
        dis                     ; disable interrupt
        dc      X'33'
        sex     R2
        scal    R4, queue_remove
        dc      A(rx_queue)
        sex     R3
        ret                     ; enable interrupt
        dc      X'33'
        sex     R2
        sret    R4

;;; Put character
;;; @param D char
;;; @unchanged D
;;; @clobber R15
putchar:
        stxd                    ; save D
        plo     R15             ; save D to scratch pad
        glo     R7              ; save R7.0
        stxd
        glo     R15             ; restore D
        ;;
        plo     R7              ; R7.0=char
putchar_loop:
        sex     R3
        dis                     ; disable interrupt
        dc      X'33'
        sex     R2
        scal    R4, queue_add
        dc      A(tx_queue)
        sex     R3
        ret                     ; enable interrupt
        dc      X'33'
        sex     R2
        bz      putchar_loop    ; retry if queue is full
        rldi    R15, putchar_txint
        sex     R15              ; R15 for out
        out     ACIA_control
putchar_exit:
        sex     R2
        irx
        ldxa                    ; restore R7.0
        plo     R7
        ldx                     ; restore D
        sret    R4
putchar_txint:
        dc      RX_INT_TX_INT

        org     X'0300'
        include "queue.inc"

;;; From scrt_isr, P=3, X=2
isr_char:
        dc      0
isr:
        rsxd    R8              ; save R8
        rsxd    R7              ; save R7
        rldi    R8, isr_char
        sex     R8              ; R8 for inp
        inp     ACIA_status
        ani     IRQF_bm
        bz      isr_exit
        inp     ACIA_status
        ani     RDRF_bm
        bz      isr_send        ; no data is received
        inp     ACIA_data
        plo     R7
        sex     R2
        scal    R4, queue_add
        dc      A(rx_queue)
isr_send:
        sex     R8              ; R8 for inp
        inp     ACIA_status
        ani     TDRE_bm
        bz      isr_exit
        sex     R2
        scal    R4, queue_remove
        dc      A(tx_queue)
        sex     R8              ; R8 for out
        bz      isr_send_empty
        glo     R7
        str     R8              ; send char
        out     ACIA_data
        br      isr_exit
isr_send_empty:
        ldi     RX_INT_TX_NO    ; disable Tx interrupt
        str     R8
        out     ACIA_control
isr_exit:
        sex     R2
        irx
        rlxa    R7              ; restore R7
        rlxa    R8              ; restore R8
        dec     R2
        sep     R1              ; return to scrt_isr
