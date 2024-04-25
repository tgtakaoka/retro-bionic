;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tlcs90
        include "tmp90c802.inc"

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

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size
tx_intr_enable: db      1

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jp      init

        org     ORG_SWI
        halt                    ; halt to system

        org     ORG_INT0
        jp      isr_intr

        org     0100H
init:
        ld      sp, stack
        ld      ix, rx_queue
        ld      b, rx_queue_size
        call     queue_init
        ld      ix, tx_queue
        ld      b, tx_queue_size
        call     queue_init
init_usart:
        ld      (USARTC), 0
        ld      (USARTC), 0
        ld      (USARTC), 0     ; safest way to sync mode
        ld      (USARTC), CMD_IR_bm
        nop
        nop
        ld      (USARTC), ASYNC_MODE
        nop
        nop
        ld      (USARTC), RX_EN_TX_DIS ; enable Rx, disable Tx
        ld      (USARTRV), ORG_INT0    ; enable Rx interrupt using INT0
        ld      (USARTTV), ORG_INT0    ; enable Tx interrupt using INT0
        set     INTEH_IE0_bp, (INTEH)  ; enable INT0
        ei

loop:
        call    mandelbrot
        call    newline
        jr      loop

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
getchar:
        push    ix
        ld      ix, rx_queue
        di
        call    queue_remove
        ei
        pop     ix
        ret

;;; Put character
;;; @param A
putchar:
        push    af
        push    ix
        ld      ix, tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jr      nc, putchar_retry ; branch if queue is full
        ld      (USARTC), RX_EN_TX_EN ; enable Tx
        ei
        pop     ix
        pop     af
        ret

newline:
        ld      A, 0DH
        call    putchar
        ld      A, 0AH
        jr      putchar

putspace:
        ld      A, ' '
        jr      putchar

isr_intr:
        bit     ST_RxRDY_bp, (USARTS)
        jr      nz, isr_intr_rx
        bit     ST_TxRDY_bp, (USARTS)
        jr      nz, isr_intr_tx
        reti

isr_intr_rx:
        push    ix
        ld      a, (USARTD)     ; receive character
        ld      ix, rx_queue
        call    queue_add
        pop     ix
        reti

isr_intr_tx:
        push    ix
        ld      ix, tx_queue
        call    queue_remove
        pop     ix
        jr      nc, isr_intr_send_empty
        ld      (USARTD), a     ; send character
        reti
isr_intr_send_empty:
        ld      (USARTC), RX_EN_TX_DIS ; disable Tx
        reti

        include "queue.inc"
        include "mandelbrot.inc"
        include "arith.inc"

        end
