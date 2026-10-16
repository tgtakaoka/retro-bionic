;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z80.inc"
        include "usart.inc"
        include "kl5c80a12.inc"

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size

        org     1000H
stack:          equ     $

        org     ORG_RESET
        jp      init

        ;; Mode 2 vector for both the USART's Rx and Tx, and for the
        ;; KL5C80A12's IR0
        org     0080H
vector:
        dw      isr_intr

        org     0100H
init:
        ld      SP, stack
        ld      HL, rx_queue
        ld      B, rx_queue_size
        call    queue_init
        ld      HL, tx_queue
        ld      B, tx_queue_size
        call    queue_init
init_usart:
        ;; IN/OUT (C) put B on A8-A15. Boards decoding 16-bit I/O
        ;; addresses find the USART at 0140H, clear of on-chip I/O; the
        ;; others decode A0-A7 alone and find it at 40H.
        ld      BC, 0100H|USARTC
        xor     A               ; clear A
        out     (C), A
        out     (C), A
        out     (C), A          ; safest way to sync mode
        ld      A, CMD_IR_bm
        out     (C), A          ; reset
        nop
        nop
        ld      A, ASYNC_MODE
        out     (C), A
        nop
        nop
        ld      A, RX_EN_TX_DIS
        out     (C), A

        ld      A, LOW vector
        ld      C, USARTRV
        out     (C), A          ; set RxRDY interrupt vector
        ld      C, USARTTV
        out     (C), A          ; set TxRDY interrupt vector
        ld      A, HIGH vector
        ld      I, A

        ;; KL5C80A12 interrupt controller; IR0 is left level triggered
        ;; as reset leaves it. OUT (n),A puts A on A8-A15, so a Z180
        ;; doesn't take these for its on-chip I/O.
        ld      A, LOW vector
        out     (IVR), A        ; vector register
        ld      A, ~1           ; disable mask for IR0
        out     (IMRL), A
        im      2

        ei

loop:
        call    mandelbrot
        call    newline
        jr      loop

;;; Get character
;;; @return A
;;; @return F.C 0 if no character
getchar:
        push    HL
        ld      HL, rx_queue
        di
        call    queue_remove
        ei
        pop     HL
        ret

;;; Put newline
;;; @clobber A
newline:
        ld      A, 0DH
        call    putchar
        ld      A, 0AH

;;; Put character
;;; @param A
putchar:
        push    AF
        push    HL
        ld      HL, tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jr      NC, putchar_retry ; branch if queue is full
        pop     HL
        push    BC
        ld      BC, 0100H|USARTC
        ld      a, RX_EN_TX_EN  ; enable Tx
        out     (C), A
        pop     BC
putchar_exit:
        pop     AF
        ret

;;; Put space
;;; @clobber A
putspace:
        ld      A, ' '
        jr      putchar

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

;;; Rx and Tx share the vector: receive if RxRDY, then send if TxRDY
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

        end
