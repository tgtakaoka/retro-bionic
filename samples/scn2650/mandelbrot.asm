;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     2650
        include "scn2650.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USARTD:         equ     H'00'           ; Data register
USARTS:         equ     H'01'           ; Status register
USARTC:         equ     H'01'           ; Control register
USARTRI:        equ     H'02'           ; Receive interrupt vector
USARTTI:        equ     H'03'           ; Transmit interrupt vector
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm + CMD_DTR_bm + CMD_RxEN_bm + CMD_TxEN_bm
RX_EN_TX_DIS:   equ     CMD_RTS_bm + CMD_DTR_bm + CMD_RxEN_bm

TXRX_ENABLE:    equ     CMD_RTS_bm + CMD_DTR_bm + CMD_RxEN_bm + CMD_TxEN_bm
RXERR_RESET:    equ     TXRX_ENABLE + CMD_ER_bm

        org     H'2000'
rx_queue_size:  equ     128
rx_queue:       res     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       res     tx_queue_size

        org     ORG_RESET
        ppsu    PSU_II          ; disable interrupt
        ppsl    PSL_WC+PSL_COM  ; with carry, logical compare
        bcta,un init

RXINTR_VEC:     equ     $
        bcta,un isr_intr_rx

TXINTR_VEC:     equ     $
        bcta,un isr_intr_tx

init:
        lodi,r2 >rx_queue
        lodi,r3 <rx_queue
        lodi,r1 rx_queue_size
        bsta,un queue_init
        lodi,r2 >tx_queue
        lodi,r3 <tx_queue
        lodi,r1 tx_queue_size
        bsta,un queue_init

init_usart:
        eorz    r0              ; clear R0
        wrte,r0 USARTC
        wrte,r0 USARTC
        wrte,r0 USARTC          ; safest way to sync mode
        lodi,r0 CMD_IR_bm
        wrte,r0 USARTC          ; reset
        nop
        nop
        lodi,r0 ASYNC_MODE
        wrte,r0 USARTC          ; async 1stop 8data x16
        nop
        nop
        lodi,r0 RX_EN_TX_DIS
        wrte,r0 USARTC    ; RTS/DTR, error reset, Rx enable, Tx disable
        lodi,r0 RXINTR_VEC
        wrte,r0 USARTRI         ; enable Rx interrupt
        lodi,r0 TXINTR_VEC
        wrte,r0 USARTTI         ; enable Tx interrupt

loop:   
        bsta,un mandelbrot
        bsta,un newline
        bctr,un loop

;;; Get character
;;; @return r0
;;; @return FLAGS.C 0 if no character
getchar:
        strr,r2 save_r2
        strr,r3 save_r3
        lodi,r2 >rx_queue
        lodi,r3 <rx_queue
        ppsu    PSU_II
        bsta,un queue_remove
        cpsu    PSU_II
        lodr,r3 save_r3
        lodr,r2 save_r2
        retc,un

;;; Put character
;;; @param r0
putchar:
        strr,r0 save_r0
        strr,r2 save_r2
        strr,r3 save_r3
        lodi,r2 >tx_queue
        lodi,r3 <tx_queue
putchar_retry:
        ppsu    PSU_II
        bsta,un queue_add
        cpsu    PSU_II
        tpsl    PSL_C
        bcfr,eq putchar_retry   ; branch if queue is full
        lodi,r0 RX_EN_TX_EN
        wrte,r0 USARTC          ; enable Tx
        lodr,r3 save_r3
        lodr,r2 save_r2
        lodr,r0 save_r0
        retc,un
save_r2:
        res     1
save_r3:
        res     1
save_r0:
        res     1

newline:
        lodi,r0 H'0D'
        bstr,un putchar
        lodi,r0 H'0A'
        bctr,un putchar

putspace:
        lodi,r0 A' '
        bctr,un putchar

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

isr_intr_rx:
        bstr,un isr_save
        rede,r0 USARTS
        tmi,r0  ST_RxRDY_bm
        bcfr,eq isr_return
        rede,r0 USARTD
        lodi,r2 >rx_queue
        lodi,r3 <rx_queue
        bsta,un queue_add
        bctr,un isr_return

isr_intr_tx:
        bstr,un isr_save
        rede,r0 USARTS
        tmi,r0  ST_TxRDY_bm
        bcfr,eq isr_return
        lodi,r2 >tx_queue
        lodi,r3 <tx_queue
        bsta,un queue_remove
        tpsl    PSL_C
        bcfr,eq isr_intr_send_empty
        wrte,r0 USARTD
        bctr,un isr_return
isr_intr_send_empty:
        lodi,r0 RX_EN_TX_DIS
        wrte,r0 USARTC          ; disable Tx
        bctr,un isr_return

isr_save:
        strr,r0 isr_context
        strr,r1 isr_context+1
        strr,r2 isr_context+2
        strr,r3 isr_context+3
        spsl
        strr,r0 isr_context+4
        retc,un

isr_context:
        res     5

isr_return:
        lodr,r3 isr_context+3
        lodr,r2 isr_context+2
        lodr,r1 isr_context+1
        lodr,r0 isr_context+4
        lpsl
        bctr,eq isr_reteq
        bctr,gt isr_retgt
        bctr,lt isr_retlt
        lodr,r0 isr_context
        ppsl    PSL_CC1+PSL_CC0
        rete,un
isr_reteq:
        lodr,r0 isr_context
        cpsl    PSL_CC1+PSL_CC0
        rete,un
isr_retgt:
        lodr,r0 isr_context
        cpsl    PSL_CC1
        ppsl    PSL_CC0
        rete,un
isr_retlt:
        lodr,r0 isr_context
        ppsl    PSL_CC1
        cpsl    PSL_CC0
        rete,un

        end
