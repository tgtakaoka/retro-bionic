;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     2650
        include "scn2650.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USARTD: equ     H'00'           ; Data register
USARTS: equ     H'01'           ; Status register
USARTC: equ     H'01'           ; Control register
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc + MODE_LEN8_gc + MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm + CMD_DTR_bm + CMD_ER_bm + CMD_RxEN_bm + CMD_TxEN_bm

        org     ORG_RESET
        ppsu    PSU_II          ; disable interrupt
        ppsl    PSL_WC+PSL_COM  ; with carry, logical compare
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
        lodi,r0 RX_EN_TX_EN
        wrte,r0 USARTC    ; RTS/DTR, error reset, Rx enable, Tx enable

receive_loop:
        rede,r1 USARTS
        tmi,r1  ST_RxRDY_bm
        bcfr,eq receive_loop
receive_data:
        rede,r0 USARTD
        bctr,eq echo_halt
transmit_loop:
        rede,r1 USARTS
        tmi,r1  ST_TxRDY_bm
        bcfr,eq transmit_loop
transmit_data:
        wrte,r0 USARTD
        comi,r0 H'0D'
        bcfr,eq receive_loop
        lodi,r0 H'0A'
        bctr,un transmit_loop
echo_halt:
        halt
