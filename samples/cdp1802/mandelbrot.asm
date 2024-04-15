;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     1802
        option  "smart-branch", "on"
        include "cdp1802.inc"

        org     ORG_RESET
        dis                     ; disable interrupt
        dc      X'00'           ; X:P=0:0
        br      scrt_init
        include "scrt.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     X'0DF00'
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

        org     X'1000'
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
        ldi     A.1(ACIA)
        phi     R8
        ldi     A.0(ACIA)
        plo     R8
        ldi     CDS_RESET_gc    ; Master reset
        str     R8              ; ACIA_control
        ldi     RX_INT_TX_NO
        str     R8              ; ACIA_control
        sex     R3
        ret
        dc      X'33'           ; enable interrupt
        sex     R2

loop:
        sep     R5              ; call mandelbrot
        dc      A(mandelbrot)
        sep     R5
        dc      A(newline)
        br      loop

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
        ldi     A.1(ACIA)
        phi     R15
        ldi     A.0(ACIA)
        plo     R15
        ldi     RX_INT_TX_INT   ; enable Tx interrupt
        str     R15             ; ACIA_C
putchar_exit:
        irx
        ldxa                    ; restore R7.0
        plo     R7
        ldx                     ; restore D
        sep     R6              ; return

;;; Print out newline
;;; @clobber D R15.0
newline:
        ldi     X'0D'
        sep     R5              ; call
        dc      A(putchar)
        ldi     X'0A'
        br      putchar

;;; Print out space
;;; @clobber D R15.0
putspace:
        ldi     T' '
        br      putchar

;;; From scrt_isr, X=2, P=3
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
        ldi     A.1(ACIA)
        phi     R8
        ldi     A.0(ACIA)
        plo     R8              ; R8=ACIA
        ldn     R8              ; ACIA_status
        ani     IRQF_bm
        bz      isr_exit
        ldn     R8              ; ACIA_status
        ani     RDRF_bm
        bz      isr_send        ; no data is received
        inc     R8
        ldn     R8              ; ACIA_data
        dec     R8
        plo     R7
        sep     R5              ; call queue_add
        dc      A(queue_add)
        dc      A(rx_queue)
isr_send:
        ldn     R8              ; ACIA_status
        ani     TDRE_bm
        bz      isr_exit
        sep     R5              ; call queue_remove
        dc      A(queue_remove)
        dc      A(tx_queue)
        bz      isr_send_empty
        glo     R7
        inc     R8
        str     R8              ; ACIA_D
        dec     R8
        br      isr_exit
isr_send_empty:
        ldi     RX_INT_TX_NO    ; disable Tx interrupt
        str     R8              ; ACIA_C
isr_exit:
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

        include "queue.inc"
        include "arith.inc"
        include "mandelbrot.inc"

        end
