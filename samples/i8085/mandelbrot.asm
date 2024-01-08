;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8085
        include "i8085.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     00H
USARTD:         equ     USART+0 ; Receive/Transmit data
USARTS:         equ     USART+1 ; Srtatus register
USARTC:         equ     USART+1 ; Control register
USARTRV:        equ     USART+2 ; Receive interrupt vector (ORG_*)
USARTTV:        equ     USART+3 ; Transmit interrupt vector (ORG_*)
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm
RX_EN_TX_DIS:   equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jmp     init

        org     ORG_RST55
        jmp     isr_intr_rx

        org     ORG_RST65
        jmp     isr_intr_tx

        org     0100H
init:
        lxi     sp, stack
        lxi     h, rx_queue
        mvi     b, rx_queue_size
        call    queue_init
        lxi     h, tx_queue
        mvi     b, tx_queue_size
        call    queue_init
init_usart:
        xra     a               ; clear A
        out     USARTC
        out     USARTC
        out     USARTC          ; safest way to sync mode
        mvi     a, CMD_IR_bm
        out     USARTC          ; reset
        nop
        nop
        mvi     a, ASYNC_MODE
        out     USARTC
        nop
        nop
        mvi     a, RX_EN_TX_DIS
        out     USARTC
        mvi     a, ORG_RST55
        out     USARTRV         ; set RxRDY interrupt vector RST 5.5
        mvi     a, ORG_RST65
        out     USARTTV         ; set TxRDY interrupt vector RST 6.5

        rim
        ani     ~(SIM_M55|SIM_M65) ; enable RST 5.5/RST 5.6
        ori     SIM_MSE|SIM_R75
        sim
        ei

        call    mandelbrot
        call    newline
        lxi     h, tx_queue
wait:   mov     a, m
        ora     a
        jnz     wait
        hlt

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
getchar:
        push    h
        lxi     h, rx_queue
        di
        call    queue_remove
        ei
        pop     h
        ret

;;; Put character
;;; @param A
putchar:
        push    psw
        push    h
        lxi     h, tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jnc     putchar_retry   ; branch if queue is full
        pop     h
        mvi     a, RX_EN_TX_EN  ; enable Tx
        out     USARTC
putchar_exit:
        pop     psw
        ret

;;; Put newline
;;; @clobber A
newline:
        mvi     A, 0DH
        call    putchar
        mvi     A, 0AH
        jmp     putchar

;;; Put newline
;;; @clobber A
putspace:
        mvi     A, ' '
        jmp     putchar

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

isr_intr_rx:
        push    psw
        in      USARTS
        ani     ST_RxRDY_bm
        jz      isr_intr_rx_exit
        in      USARTD          ; receive character
        push    h
        lxi     h, rx_queue
        call    queue_add
        pop     h
isr_intr_rx_exit:
        pop     psw
        ei
        ret

isr_intr_tx:
        push    psw
        in      USARTS
        ani     ST_TxRDY_bm
        jz      isr_intr_tx_exit
        push    h
        lxi     h, tx_queue
        call    queue_remove
        pop     h
        jnc     isr_intr_send_empty
        out     USARTD          ; send character
isr_intr_tx_exit:
        pop     psw
        ei
        ret
isr_intr_send_empty:
        mvi     a, RX_EN_TX_DIS
        out     USARTC          ; disable Tx
        pop     psw
        ei
        ret

        end
