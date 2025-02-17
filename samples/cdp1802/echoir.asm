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
ACIA:   equ     4
        include "mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

        org     X'2000'

rx_queue_size:  equ     128
rx_queue:
        org     *+rx_queue_size


stack:  equ     X'1000'-1

        org     X'0100'
main:
        sep     R5
        dc      A(queue_init)   ; call queue_init
        dc      A(rx_queue)
        dc      rx_queue_size
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
        sex     R3
        dis                     ; disable interrupt
        dc      X'33'           ; X=3, P=3
        sex     R2
        sep     R5              ; call queue_remove
        dc      A(queue_remove)
        dc      A(rx_queue)
        sex     R3
        ret                     ; enable interrupt
        dc      X'33'
        sex     R2
        bz      loop            ; branch if queue is empty
        glo     R7
        bz      halt_to_system
echo:
        sep     R5              ; call putchar
        dc      A(putchar)
        glo     R7
        xri     X'0D'           ; carriage return
        bnz     loop
        ldi     X'0A'           ; newline
        plo     R7
        br      echo
halt_to_system:
        idl

;;; @param D char
;;; @unchanged D
;;; @clobber R15
putchar_char:
        dc      0
putchar:
        plo     R15             ; save D to R15.0
        glo     R8              ; save R8
        stxd
        ghi     R8
        stxd
        ;;
        ldi     A.1(putchar_char)
        phi     R8
        ldi     A.0(putchar_char)
        plo     R8
        sex     R8              ; R8 for inp/out
putchar_loop:
        inp     ACIA_status
        ani     TDRE_bm
        bz      putchar_loop
        glo     R15             ; restore D
        str     R8              ; putchar_char
        out     ACIA_data
        ;;
        sex     R2
        irx
        ldxa                    ; restore R8
        phi     R8
        ldx
        plo     R8
        glo     R15             ; restore D
        sep     R6              ; return

        include "queue.inc"

;;; From scrt_isr, P=3
isr_char:
        dc      0
isr:
        glo     R8              ; save R8
        stxd
        ghi     R8
        stxd
        glo     R7
        stxd                    ; save R7.0
        ;;
        ldi     A.1(isr_char)
        phi     R8
        ldi     A.0(isr_char)
        plo     R8
        sex     R8              ; R8 for inp/out
        inp     ACIA_status
        ani     IRQF_bm
        bz      isr_exit        ; no interrupt
isr_receive:
        inp     ACIA_status
        ani     RDRF_bm
        bz      isr_exit        ; no data is received
        inp     ACIA_data
        plo     R7
        sep     R5              ; call queue_add
        dc      A(queue_add)
        dc      A(rx_queue)
isr_exit:
        sex     R2
        irx
        ldxa                    ; restore R7.0
        plo     R7
        ldxa                    ; restore R8
        phi     R8
        ldx
        plo     R8
        sep     R1              ; return to scrt_isr
