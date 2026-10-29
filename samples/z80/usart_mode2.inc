;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Mode 2 RxRDY and TxRDY interrupt of the i8251, sharing one vector:
;;; queue the received character, then send the next queued one, or
;;; disable Tx when the queue is empty. IN/OUT (C) put B on A8-A15, so
;;; boards decoding 16-bit I/O addresses find the USART at 0140H.
        cpu     z80
isr_intr:
        push    AF
        push    BC
        ld      BC, 0100H|USARTS
        in      A, (C)
        bit     ST_RxRDY_bp, A
        jr      Z, isr_intr_tx
isr_intr_rx:
        ld      C, USARTD
        in      A, (C)          ; receive character
        push    HL
        ld      HL, rx_queue
        call    queue_add
        pop     HL
        ld      C, USARTS
        in      A, (C)
isr_intr_tx:
        bit     ST_TxRDY_bp, A
        jr      Z, isr_intr_exit
        push    HL
        ld      HL, tx_queue
        call    queue_remove
        pop     HL
        jr      NC, isr_intr_send_empty
        ld      C, USARTD
        out     (C), A          ; send character
isr_intr_exit:
        pop     BC
        pop     AF
        ei
        reti
isr_intr_send_empty:
        ld      A, RX_EN_TX_DIS
        ld      C, USARTC
        out     (C), A          ; disable Tx
        jr      isr_intr_exit

