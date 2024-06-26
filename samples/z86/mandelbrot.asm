;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z86c
        option  "reg-alias", "disable"

        include "z8.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     %FF00
USARTD:         equ     USART+0 ; Receive/Transmit data
USARTS:         equ     USART+1 ; Status register
USARTC:         equ     USART+1 ; Control register
USARTRI:        equ     USART+2 ; Receive interrupt name (IRQ0~2)
USARTTI:        equ     USART+3 ; Transmit interrupt name (IRQ0~2)
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc LOR MODE_LEN8_gc LOR MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm LOR CMD_TxEN_bm
RX_EN_TX_DIS:   equ     CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm

        org     %2000
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size

;;; work area for mandelbrot.inc
        org     %40
c229:   ds      2
c416:   ds      2
c100:   ds      2
vF:     ds      2
vC:     ds      2
vD:     ds      2
vA:     ds      2
vB:     ds      2
vP:     ds      2
vQ:     ds      2
vS:     ds      2
vT:     ds      2
tmp:    ds      2
vY:     ds      2
vX:     ds      2
vI:     ds      2

        org     %1000
stack:  equ     $

        org     VEC_IRQ0
        dw      isr_intr_rx

        org     VEC_IRQ1
        dw      isr_intr_tx

        org     ORG_RESET
        setrp   -1
init_config:
        ;; Stack is on external memory
        ld      P01M, #P01M_P0ADDR LOR P01M_P1DATA
        ld      P2M, #%FF       ; Port 2 is input
        ld      SPH, #HIGH stack
        ld      SPL, #LOW stack
        srp     #%10
        setrp   %10
        ld      R2, #HIGH rx_queue
        ld      R3, #LOW rx_queue
        ld      R1, #rx_queue_size
        call    queue_init
        ld      R2, #HIGH tx_queue
        ld      R3, #LOW tx_queue
        ld      R1, #tx_queue_size
        call    queue_init

init_usart:
        ld      R12, #HIGH USARTC
        ld      R13, #LOW USARTC
        clr     R0
        lde     @RR12, R0
        lde     @RR12, R0
        lde     @RR12, R0       ; safest way to sync mode
        ld      R0, #CMD_IR_bm
        lde     @RR12, R0       ; reset
        nop
        nop
        ld      R0, #ASYNC_MODE
        lde     @RR12, R0       ; async 1stop 8data x16
        nop
        nop
        ld      R0, #RX_EN_TX_DIS
        lde     @RR12, R0       ; RTS/DTR, error reset, Rx enable, Tx disable
        ld      R0, #INTR_IRQ0
        ld      R13, #LOW USARTRI
        lde     @RR12, R0       ; enable RxRDY interrupt using IRQ0
        ld      R0, #INTR_IRQ1
        ld      R13, #LOW USARTTI
        lde     @RR12, R0       ; enable TxRDY interrupt using IRQ1

        ld      IPR, #IPR_BCA LOR IPR_B02 LOR IPR_C14 LOR IPR_A35
        ;; enable IRQ0 and IRQ1
        ld      IMR, #IMR_IRQ0 LOR IMR_IRQ1
        ei

loop:
        call    mandelbrot
        call    newline
        jr      loop

;;; Get character
;;; @return R0
;;; @return FLAGS.C 0 if no character
getchar:
        push    R3
        push    R2
        ld      R2, #HIGH rx_queue
        ld      R3, #LOW rx_queue
        di
        call    queue_remove
        ei
        pop     R2
        pop     R3
        ret

;;; Put character
;;; @param R0
putchar:
        push    R0
        push    R2
        push    R3
        ld      R2, #HIGH tx_queue
        ld      R3, #LOW tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jr      nc, putchar_retry ; branch if queue is full
        ld      R2, #HIGH USARTC
        ld      R3, #LOW USARTC
        ld      R0, #RX_EN_TX_EN
        lde     @RR2, R0        ; enable Tx
putchar_exit:
        pop     R3
        pop     R2
        pop     R0
        ret

newline:
        ld      R0, #%0D
        call    putchar
        ld      R0, #%0A
        jr      putchar

putspace:
        ld      R0, #' '
        jr      putchar

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

        setrp   -1
isr_intr_rx:
        push    R0
        push    R2
        push    R3
        ld      R2, #HIGH USARTS
        ld      R3, #LOW USARTS
        lde     R0, @RR2        ; USARTS
        and     R0, #ST_RxRDY_bm
        jr      z, isr_intr_rx_exit
        ld      R3, #LOW USARTD
        lde     R0, @RR2        ; USARTD
        ld      R2, #HIGH rx_queue
        ld      R3, #LOW rx_queue
        call    queue_add
isr_intr_rx_exit:
        pop     R3
        pop     R2
        pop     R0
        iret

isr_intr_tx:
        push    R0
        push    R2
        push    R3
        ld      R2, #HIGH USARTS
        ld      R3, #LOW USARTS
        lde     R0, @RR2        ; USARTS
        and     R0, #ST_TxRDY_bm
        jr      z, isr_intr_tx_exit
        ld      R2, #HIGH tx_queue
        ld      R3, #LOW tx_queue
        call    queue_remove
        ld      R2, #HIGH USARTD
        ld      R3, #LOW USARTD
        jr      nc, isr_intr_send_empty
        lde     @RR2, R0        ; USARTD
isr_intr_tx_exit:
        pop     R3
        pop     R2
        pop     R0
        iret
isr_intr_send_empty:
        ld      R3, #LOW USARTC
        ld      R0, #RX_EN_TX_DIS
        lde     @RR2, R0        ; disable Tx
        pop     R3
        pop     R2
        pop     R0
        iret

        end
