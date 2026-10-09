;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     140H
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
USARTRV:        equ     USART+2 ; Receive interrupt identifier
USARTTV:        equ     USART+3 ; Transmit interrupt identifier
;;; The identifiers the USART answers its vectored interrupts with: even,
;;; as the Z8001 requires.
RX_ID:          equ     2
TX_ID:          equ     4
        include "../z80/i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm
RX_EN_TX_DIS:   equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm
