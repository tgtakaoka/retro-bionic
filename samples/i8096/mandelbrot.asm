;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "i8096.inc"
        include "usart.inc"

        org     3000H
rx_queue_size:  equ     128
rx_queue:       dsb     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       dsb     tx_queue_size

        org     1000H
stack:          equ     $

        org     VEC_EXTINT
        dcw     isr_intr

        org     1AH
A:      dsb     1
B:      dsb     1
HL:     dsw     1
QHL:    dsw     1               ; 32-bit for mul/div
DE:     dsw     1

;;; mandelbrot workspace
vC:     dcw     1
vD:     dcw     1
vA:     dcw     1
vB:     dcw     1
vP:     dcw     1
vQ:     dcw     1
vS:     dcw     1
vT:     dcw     1
vY:     dsb     1
vX:     dsb     1
vI:     dsb     1

        org     ORG_RESET
init:
        ld      SP, #stack
        ldb     INT_MASK, INT_EXTINT ; enable EXTINT

        ld      HL, #rx_queue
        ldb     B, #rx_queue_size
        scall   queue_init
        ld      HL, #tx_queue
        ldb     B, #tx_queue_size
        scall   queue_init
init_usart:
        stb     ZERO, USARTC
        stb     ZERO, USARTC
        stb     ZERO, USARTC    ; safest way to sync mode
        ldb     A, #CMD_IR_bm
        stb     A, USARTC       ; reset
        nop
        nop
        ldb     A, #ASYNC_MODE
        stb     A, USARTC
        nop
        nop
        ldb     A, #RX_EN_TX_EN
        stb     A, USARTC
        ldb     A, #1
        stb     A, USARTRV      ; enable receive interrupt
        stb     A, USARTTV      ; enable transmit interrupt
        ei

loop:
        scall   mandelbrot
        scall   newline
        sjmp    loop

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
getchar:
        push    HL
        ld      HL, #rx_queue
        di
        scall   queue_remove
        ei
        pop     HL
        ret

;;; Put newline
;;; @clobber A
newline:
        ldb     A, #0DH
        scall   putchar
        ldb     A, #0AH
        sjmp    putchar

;;; Put spcase
;;; @clobber A
putspace:
        ldb     A, #' '

;;; Put character
;;; @param A
putchar:
        push    A
        push    HL
        ld      HL, #tx_queue
putchar_retry:
        di
        scall   queue_add
        ei
        jnc     putchar_retry   ; branch if queue is full
        pop     HL
        ldb     A, #RX_EN_TX_EN ; enable Tx
        stb     A, USARTC
        pop     A
        ret

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"


isr_intr:
        pushf
        push    A               ; push A,B
        push    HL
        ldb     A, USARTS
        jbc     A, ST_RxRDY_bp, isr_intr_tx
        ldb     A, USARTD       ; receive character
        ld      HL, #rx_queue
        scall   queue_add
isr_intr_tx
        ldb     A, USARTS
        jbc     A, ST_TxRDY_bp, isr_intr_exit
        ld      HL, #tx_queue
        scall   queue_remove
        jnc     isr_intr_send_empty
        stb     A, USARTD       ; send character
isr_intr_exit:
        pop     HL
        pop     A
        popf
        ret
isr_intr_send_empty:
        ldb     A, #RX_EN_TX_DIS
        stb     A, USARTC       ; disable Tx
        sjmp    isr_intr_exit

        end
