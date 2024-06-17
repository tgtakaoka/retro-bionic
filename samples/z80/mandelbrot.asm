;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z80
        include "z80.inc"

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
stack:          equ     $

vec_base:       equ     $
        org     vec_base+12H
vec_rx: dw      isr_intr_rx
        org     vec_base+8AH
vec_tx: dw      isr_intr_tx

        org     ORG_RESET
        jp      init

        org     ORG_RST28
        jp      isr_intr_rx

        org     ORG_RST30
        jp      isr_intr_tx

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

        db      3EH             ; LD A, n
        rst     28H
        out     (USARTRV), A    ; set RxRDY interrupt vector RST 28H
        db      3EH             ; LD A, n
        rst     30H
        out     (USARTTV), A    ; set TxRDY interrupt vector RST 30H
        im      0

        ;; ld      A, HIGH vec_base
        ;; ld      I, A
        ;; ld      A, LOW vec_rx
        ;; out     (USARTRV), A    ; set RxRDY interrupt vec_rx
        ;; ld      A, LOW vec_tx
        ;; out     (USARTTV), A    ; set TxRDY interrupt vec_tx
        ;; im      2

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
putchar_exit:
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

isr_intr_rx:
        push    AF
        in      A, (USARTS)
        bit     ST_RxRDY_bp, A
        jr      Z, isr_intr_rx_exit
        in      A, (USARTD)     ; receive character
        push    HL
        ld      HL, rx_queue
        call    queue_add
        pop     HL
isr_intr_rx_exit:
        pop     AF
        ei
        reti

isr_intr_tx:
        push    AF
        in      A, (USARTS)
        bit     ST_TxRDY_bp, A
        jr      Z, isr_intr_tx_exit
        push    HL
        ld      HL, tx_queue
        call    queue_remove
        pop     HL
        jr      NC,isr_intr_send_empty
        out     (USARTD), A     ; send character
isr_intr_tx_exit:
        pop     AF
        ei
        reti
isr_intr_send_empty:
        ld      a, RX_EN_TX_DIS
        out     (USARTC), A     ; disable Tx
        pop     AF
        ei
        reti

        end
