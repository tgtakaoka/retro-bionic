;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     1802
        option  "smart-branch", "on"
        include "cdp1802.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     4
        include "mc6850.inc"

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
        sep     R5
        dc      A(queue_init)   ; call queue_init
        dc      A(rx_queue)
        dc      rx_queue_size
        sep     R5
        dc      A(queue_init)   ; call queue_init
        dc      A(tx_queue)
        dc      tx_queue_size
        ;; initialize ACIA
        ldi     A.1(ACIA_config)
        phi     R8
        ldi     A.0(ACIA_config)
        plo     R8
        sex     R8              ; R8 for inp/out
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
        sep     R5              ; call getchar
        dc      A(getchar)
        bz      loop
        glo     R7
        bnz     echo_back
        idl
echo_back:
        sep     R5              ; call putchar
        dc      A(putchar)
        ldi     ' '
        sep     R5              ; call putchar
        dc      A(putchar)
        glo     R7
        sep     R5              ; call put_hex8
        dc      A(put_hex8)
        ldi     ' '
        sep     R5              ; call putchar
        dc      A(putchar)
        glo     R7
        sep     R5              ; call put_bin8
        dc      A(put_bin8)
        ldi     x'0d'
        sep     R5              ; call putchar
        dc      A(putchar)
        ldi     x'0a'
        sep     R5              ; call putchar
        dc      A(putchar)
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
        sep     R5              ; call putchar
        dc      A(putchar)
        ldi     'x'
        sep     R5              ; call putchar
        dc      A(putchar)
        glo     R7
        shr
        shr
        shr
        shr
        sep     R5              ; call put_hex4
        dc      A(put_hex4)
        glo     R7
        sep     R5              ; call put_hex4
        dc      A(put_hex4)
        ;;
        irx
        ldx                     ; restore R7.0
        plo     R7
        sep     R6              ; return

;;; Print hexadecimal digit
;;; @param D nibble
;;; @clobber D
put_hex4:
        ani     0Fh
        smi     10
        bnf     put_hex4_dec    ; branch if D < 10
        adi     'A'
        lskp
put_hex4_dec:
        adi     '0'+10
        sep     R5              ; call putchar
        dc      A(putchar)
        sep     R6              ; return

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
        sep     R5
        dc      A(putchar)
        ldi     'b'
        sep     R5
        dc      A(putchar)
        ;;
        sep     R5
        dc      A(put_bin4)
        sep     R5
        dc      A(put_bin4)
        ;;
        irx
        ldx
        plo     R7              ; restore R7
        sep     R6              ; return
put_bin4:
        sep     R5
        dc      A(put_bin2)
put_bin2:
        sep     R5
        dc      A(put_bin1)
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
        sep     R5              ; call queue_remove
        dc      A(queue_remove)
        dc      A(rx_queue)
        sex     R3
        ret                     ; enable interrupt
        dc      X'33'
        sex     R2
        sep     R6              ; return

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
        sep     R5              ; call queue_add
        dc      A(queue_add)
        dc      A(tx_queue)
        sex     R3
        ret                     ; enable interrupt
        dc      X'33'
        sex     R2
        bz      putchar_loop    ; retry if queue is full
        ldi     A.1(putchar_txint)
        phi     R15
        ldi     A.0(putchar_txint)
        plo     R15
        sex     R15             ; R15 out output
        out     ACIA_control
putchar_exit:
        sex     R2
        irx
        ldxa                    ; restore R7.0
        plo     R7
        ldx                     ; restore D
        sep     R6              ; return
putchar_txint:
        dc      RX_INT_TX_INT

        org     X'0300'
        include "queue.inc"

;;; From scrt_isr, X=2, P=3
isr_char:
        dc      0
isr:
        glo     R8              ; save R8
        stxd
        ghi     R8
        stxd
        glo     R7              ; save R7
        stxd
        ghi     R7
        stxd
        ;;
        ldi     A.1(isr_char)
        phi     R8
        ldi     A.0(isr_char)
        plo     R8
        sex     R8              ; R8 for inp/out
        inp     ACIA_status
        ani     IRQF_bm
        bz      isr_exit
        inp     ACIA_status
        ani     RDRF_bm
        bz      isr_send        ; no data is received
        inp     ACIA_data
        plo     R7
        sep     R5              ; call queue_add
        dc      A(queue_add)
        dc      A(rx_queue)
isr_send:
        sex     R8              ; R8 for inp/out
        inp     ACIA_status
        ani     TDRE_bm
        bz      isr_exit
        sep     R5              ; call queue_remove
        dc      A(queue_remove)
        dc      A(tx_queue)
        bz      isr_send_empty
        glo     R7
        str     R8              ; output char
        sex     R8              ; R8 for inp/out
        out     ACIA_data
        br      isr_exit
isr_send_empty:
        ldi     RX_INT_TX_NO    ; disable Tx interrupt
        str     R8              ; output byte
        sex     R8              ; R8 for inp/out
        out     ACIA_control
isr_exit:
        sex     R2
        irx
        ldxa                    ; restore R7
        phi     R7
        ldxa
        plo     R7
        ldxa                    ; restore R8
        phi     R8
        ldx
        plo     R8
        sep     R1              ; return to scrt_isr

        org     ORG_RESET
        dis                     ; disable interrupt
        dc      X'00'           ; X:P=0:0
        br      scrt_init

        include "scrt.inc"
