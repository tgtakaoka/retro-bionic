;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "cp1600.inc"
;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     X'FFF0'
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

        org     X'0200'
stack:

        org     RESET_PC
initialize:
        mvii    stack, r6
        jsr     r5, init_usart
loop:   jsr     r5, getchar
        tstr    r0
        beq     halt_to_system  ; halt on NUL
echo:   jsr     r5, putchar
        cmpi    X'0D', r0
        bneq    loop            ; branch if R0!=CR
        mvii    X'0A', r0       ; NL
        b       echo
halt_to_system:
        hlt
        b       halt_to_system  ; HLT stops after this

init_usart:
        clrr    r0
        mvo     r0, USARTC
        mvo     r0, USARTC
        mvo     r0, USARTC      ; Safest way to sync mode
        mvii    CMD_IR_bm, r0
        mvo     r0, USARTC      ; Reset
        nop
        nop
        mvii    ASYNC_MODE, r0
        mvo     r0, USARTC
        nop
        nop
        mvii    RX_EN_TX_EN, r0
        mvo     r0, USARTC      ; Enable Rx/Tx
        jr      r5

;;; @return R0 received char
getchar:
        mvi     USARTS, r0
        andi    ST_RxRDY_bm, r0
        beq     getchar
        mvi     USARTD, r0
        jr      r5

;;; @param R0 char to send
putchar:
        pshr    r0
putchar_wait:
        mvi     USARTS, r0
        andi    ST_TxRDY_bm, r0
        beq     putchar_wait
        pulr    r0
        mvo     r0, USARTD
        jr      r5
