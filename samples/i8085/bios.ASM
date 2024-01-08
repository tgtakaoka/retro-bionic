        cpu     8085
        include "i8085.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     00H
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
        include "i8251.inc"

        org     1000H
stack:  equ     $

        org     ORG_RESET
        lxi     sp, stack
        call    init_usart
        jmp     0e000H

;;; NAS-SYS SCAL entry
        org     ORG_RST3
        jmp     SCAL

;;; NAS-SYS ROUT entry
        org     ORG_RST6
        jmp     putchar

SCAL:
        xthl
        mov     a,m
        inx     h
        xthl
        cpi     0BH
        jz      BLINK
        cpi     62H
        jz      RIN
        cpi     5BH
        jz      MRET
        ret

;;; Get input in A
BLINK:
        jmp     getchar

;;; Scan input
;;; Return immediately with NC if no input
;;; Return with C and input in A
RIN:
        in      USARTS
        rrc
        rrc
        rnc
        in      USARTD
        ret

MRET:
        jmp     MRET

init_usart:
        xra     a               ; clear A
        out     USARTC
        out     USARTC
        out     USARTC          ; safest way to sync mode
        mvi     a, CMD_IR_bm
        out     USARTC          ; reset
        nop
        nop
        mvi     a, MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
        out     USARTC          ; async 1stop 8data x16
        nop
        nop
        mvi     a, CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm
        out     USARTC          ; RTS/DTR, error reset, Rx enable, Tx enable
        ret

getchar:
        in      USARTS
        ani     ST_RxRDY_bm
        jz      getchar
        in      USARTD
        ret

putchar:
        push    psw
putchar_loop:
        in      USARTS
        ani     ST_TxRDY_bm
        jz      putchar_loop
        pop     psw
        out     USARTD
        cpi     0DH
        rnz
        push    psw
        mvi     a,0AH
        call    putchar
        pop     psw
        ret
