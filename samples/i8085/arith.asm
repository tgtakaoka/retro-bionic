;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "i8085.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     00H
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jmp     init_usart

        org     0100H
init_usart:
        lxi     sp, stack
        xra     A               ; clear A
        out     USARTC
        out     USARTC
        out     USARTC          ; safest way to sync mode
        mvi     A, CMD_IR_bm
        out     USARTC          ; reset
        nop
        nop
        mvi     A, ASYNC_MODE
        out     USARTC
        nop
        nop
        mvi     A, RX_EN_TX_EN
        out     USARTC

        call    arith
        hlt

putchar:
        push    PSW
putchar_loop:
        in      USARTS
        ani     ST_TxRDY_bm
        jz      putchar_loop
        pop     PSW
        out     USARTD
        ret

newline:
        push    PSW
        mvi     A, 0DH
        call    putchar
        mvi     A, 0AH
        call    putchar
        pop     PSW
        ret

putspace:
        push    PSW
        mvi     A, ' '
        call    putchar
        pop     PSW
        ret

;;; Print "v1 op v2 = "
;;; @param v1 BC
;;; @param v2 DE
;;; @param op A
;;; @return v1 HL
;;; @clobber A
expr:
        push    PSW
        mov     H, B
        mov     L, C
        call    print_int16
        call    putspace
        pop     PSW
        call    putchar
        call    putspace
        mov     H, D
        mov     L, E
        call    print_int16
        mov     H, B
        mov     L, C
        ret

;;; Print " v1\n"
;;; @param v1 HL
;;; @clobber A HL
answer:
        call    putspace
        mvi     A, '='
        call    putchar
        call    putspace
        call    print_int16
        jmp     newline

;;; Compare and print "v1 rel v2\n"
;;; @param v1 BC
;;; @param v2 DE
;;; @clobber A HL
comp:
        push    B
        push    D
        call    cmp16
        jz      comp_eq
        jp      comp_gt
        jm      comp_lt
        mvi     A, '?'
        jmp     comp_out
comp_gt:
        mvi     A, '>'
        jmp     comp_out
comp_eq:
        mvi     A, '='
        jmp     comp_out
comp_lt:
        mvi     A, '<'
comp_out:
        pop     D
        pop     B
        call    expr
        jmp     newline

        org     0200H

        org     1000H

arith:
        lxi     B, 0
        lxi     D, -28000
        mvi     A, '-'
        call    expr
        call    neg_DE
        mov     H, D
        mov     L, E
        call    answer          ; 28000

        lxi     B, 0
        lxi     D, 28000
        mvi     A, '-'
        call    expr
        call    neg_DE
        mov     H, D
        mov     L, E
        call    answer          ; -28000

        lxi     B, 18000
        lxi     D, 28000
        mvi     A, '+'
        call    expr
        dad     D
        call    answer          ; -19536

        lxi     B, 18000
        lxi     D, -18000
        mvi     A, '+'
        call    expr
        dad     D
        call    answer          ; 0

        lxi     B, -18000
        lxi     D, -18000
        mvi     A, '+'
        call    expr
        dad     D
        call    answer          ; 29536

        lxi     B, -18000
        lxi     D, -28000
        mvi     A, '-'
        call    expr
        call    neg_DE
        dad     D
        call    answer          ; -10000

        lxi     B, 100
        lxi     D, 300
        mvi     A, '*'
        call    expr
        call    mul16
        call    answer          ; 30000

        lxi     B, 300
        lxi     D, -200
        mvi     A, '*'
        call    expr
        call    mul16
        call    answer          ; 5536

        lxi     B, 100
        lxi     D, -300
        mvi     A, '*'
        call    expr
        call    mul16
        call    answer          ; -30000

        lxi     B, -200
        lxi     D, -100
        mvi     A, '*'
        call    expr
        call    mul16
        call    answer          ; 20000

        lxi     B, 30000
        lxi     D, 100
        mvi     A, '/'
        call    expr
        call    div16
        call    answer          ; 30

        lxi     B, -200
        lxi     D, 100
        mvi     A, '/'
        call    expr
        call    div16
        call    answer          ; -2

        lxi     B, -30000
        lxi     D, -200
        mvi     A, '/'
        call    expr
        call    div16
        call    answer          ; 150

        lxi     B, -30000
        lxi     D, 78
        mvi     A, '/'
        call    expr
        call    div16
        call    answer          ; -384

        lxi     B, -48
        lxi     D, 30
        call    comp

        lxi     B, 30
        lxi     D, -48
        call    comp

        lxi     B, 5000
        lxi     D, 4000
        call    comp

        lxi     B, 5000
        lxi     D, 5000
        call    comp

        lxi     B, 4000
        lxi     D, 5000
        call    comp

        lxi     B, -5000
        lxi     D, -4000
        call    comp

        lxi     B, -5000
        lxi     D, -5000
        call    comp

        lxi     B, -4000
        lxi     D, -5000
        call    comp

        lxi     B, 32700
        lxi     D, 32600
        call    comp

        lxi     B, 32700
        lxi     D, 32700
        call    comp

        lxi     B, 32600
        lxi     D, 32700
        call    comp

        lxi     B, -32700
        lxi     D, -32600
        call    comp

        lxi     B, -32700
        lxi     D, -32700
        call    comp

        lxi     B, -32600
        lxi     D, -32700
        call    comp

        lxi     B, 18000
        lxi     D, -28000
        call    comp

        lxi     B, 18000
        lxi     D, 18000
        call    comp

        lxi     B, -28000
        lxi     D, 18000
        call    comp

        ret

        include "arith.inc"

        end
