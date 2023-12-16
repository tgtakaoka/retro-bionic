;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     f3850

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     0FFF0H
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

        org     1000H
stack:

        org     2000H
rx_queue_size:  equ     128
rx_queue:       rs      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       rs      tx_queue_size

        org     0
        jmp     init

rx_vec:
        jmp     isr_intr_rx

tx_vec:
        jmp     isr_intr_tx

        org     H'0200'
init:
        pi      init_stack
        dci     rx_queue
        lr      Q, DC
        li      rx_queue_size
        lr      0, A
        pi      call
        da      queue_init
        dci     tx_queue
        lr      Q, DC
        li      tx_queue_size
        lr      0, A
        pi      call
        da      queue_init
init_usart:
        lis     0
        out     USARTC
        out     USARTC
        out     USARTC          ; safest way to sync mode
;;; reset
        li      CMD_IR_bm
        out     USARTC
        nop
        nop
        li      ASYNC_MODE
        out     USARTC
        nop
        nop
        li      RX_EN_TX_DIS
        out     USARTC
        li      rx_vec
        out     USARTRV
        li      tx_vec
        out     USARTTV

        pi      call
        da      mandelbrot
        pi      call
        da      newline
        dc      H'2F'

;;; Get character
;;; @return 0
;;; @return CC.C 0 if no character
getchar:
        ei
        nop
        di
        dci     rx_queue
        lr      Q, DC
        jmp     queue_remove

;;; Put character
;;; @param 0
;;; @clobber 0
putchar:
        ei
        nop
        di
        dci     tx_queue
        lr      Q, DC
        pi      call
        da      queue_add
        bnc     putchar         ; branch if queue is full
        li      RX_EN_TX_EN     ; enable Tx
        out     USARTC
        jmp     return

newline:        
        li      H'0D'
        lr      0, A
        pi      call
        da      putchar
        li      H'0A'
        lr      0,A
        jmp     putchar

putspace:
        li      C' '
        lr      0, A
        jmp     putchar

isr_intr_rx:
        lr      K, P
        pi      pushK
        pi      push0
        in      USARTS
        ni      ST_RxRDY_bm
        bz      isr_intr_end
        in      USARTD
        lr      0, A
        dci     rx_queue
        lr      Q, DC
        pi      call
        da      queue_add
isr_intr_end:
        pi      pop0
        jmp     return

isr_intr_tx:
        lr      K, P
        pi      pushK
        pi      push0
        dci     tx_queue
        lr      Q, DC
        pi      call
        da      queue_remove
        bnc     isr_intr_send_empty
        lr      A, 0
        out     USARTD          ; send character
        pi      pop0
        jmp     return
isr_intr_send_empty:
        li      RX_EN_TX_DIS    ; disable Tx
        out     USARTC
        pi      pop0
        jmp     return

        include "stack.inc"
        include "queue.inc"
        include "arith.inc"
        include "mandelbrot.inc"
