;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "kl5c80a12.inc"
        include "usart.inc"

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size

        org     1000H
stack:          equ     $

        org     0080H
vector:
        dw      isr_intr        ; IR0

        org     ORG_RESET
        jp      init

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
        xor     A               ; clear A
        out     (USARTC), A
        out     (USARTC), A
        out     (USARTC), A     ; safest way to sync mode
        ld      A, CMD_IR_bm
        out     (USARTC), A     ; reset
        nop
        nop
        ld      A, ASYNC_MODE
        out     (USARTC), A
        nop
        nop
        ld      A, RX_EN_TX_DIS
        out     (USARTC), A

        ld      a, 1            ; enable interrupt on USART
        out     (USARTRV), a
        out     (USARTTV), a
        ld      a, 0            ; select level trigger
        out     (LERL), a
        ld      a, high vector
        ld      i, a
        ld      a, low vector
        out     (IVR), a        ; vector register
        ld      a, ~1           ; disable mask for IR0
        out     (IMRL), a       ;
        im      2               ; mode 2 only
        ei

loop:
        call    mandelbrot
        call    newline
        jr      loop

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
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
        ld      a, RX_EN_TX_EN  ; enable Tx
        out     (USARTC), A
        pop     AF
        ret

;;; Put newline
;;; @clobber A
putspace:
        ld      A, ' '
        jr      putchar

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

isr_intr:
        push    AF
        push    HL
        in      A, (USARTS)
        bit     ST_RxRDY_bp, A
        jr      Z, isr_intr_tx
isr_intr_rx                     ;
        in      A, (USARTD)     ; receive character
        ld      HL, rx_queue
        call    queue_add
isr_intr_tx:
        in      A, (USARTS)
        bit     ST_TxRDY_bp, A
        jr      Z, isr_intr_exit
        ld      HL, tx_queue
        call    queue_remove
        jr      NC,isr_intr_send_empty
        out     (USARTD), A     ; send character
isr_intr_exit:
        pop     HL
        pop     AF
        ei
        reti
isr_intr_send_empty:
        ld      a, RX_EN_TX_DIS
        out     (USARTC), A     ; disable Tx
        jr      isr_intr_exit

        end
