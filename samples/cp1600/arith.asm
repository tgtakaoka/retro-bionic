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
        jsr     r5, arith
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

;;; Put a space (putspace), CR LF (newline) or char R0 (putchar)
putspace:
        mvii    ' ', r0
        b       putchar
newline:
        pshr    r5
        mvii    X'0D', r0
        jsr     r5, putchar
        mvii    X'0A', r0
        pulr    r5
putchar:
        pshr    r0
putchar_wait:
        mvi     USARTS, r0
        andi    ST_TxRDY_bm, r0
        beq     putchar_wait
        pulr    r0
        mvo     r0, USARTD
        jr      r5

        include "arith.inc"

;;; Print "VA op VB"
;;; @param R0 op letter
;;; @param R1 VA
;;; @param R2 VB
;;; @clobber R0
expr:
        pshr    r5
        pshr    r1
        pshr    r2
        pshr    r0
        movr    r1, r0
        jsr     r5, print_int16 ; print VA
        jsr     r5, putspace
        pulr    r0
        jsr     r5, putchar     ; print op
        jsr     r5, putspace
        pulr    r2
        pshr    r2
        movr    r2, r0
        jsr     r5, print_int16 ; print VB
        pulr    r2
        pulr    r1
        pulr    r7

;;; Print " = value\n"
;;; @param R1 value
;;; @clobber R0 R1 R2
answer:
        pshr    r5
        jsr     r5, putspace
        mvii    '=', r0
        jsr     r5, putchar
        jsr     r5, putspace
        movr    r1, r0
        jsr     r5, print_int16 ; print value
        pulr    r5
        b       newline

;;; Print "VA rel VB\n"
;;; @param R1 VA
;;; @param R2 VB
;;; @clobber R0
comp:
        pshr    r5
        mvii    '=', r0
        cmpr    r2, r1
        beq     comp_out        ; branch if VA=VB
        mvii    '>', r0
        bgt     comp_out        ; branch if VA>VB
        mvii    '<', r0
comp_out:
        jsr     r5, expr        ; print "VA rel VB"
        pulr    r5
        b       newline

arith:
        pshr    r5
        mvii    18000, r1
        mvii    28000, r2
        mvii    '+', r0
        jsr     r5, expr
        addr    r2, r1          ; R1+=R2
        jsr     r5, answer      ; -19536

        mvii    18000, r1
        mvii    -18000, r2
        mvii    '+', r0
        jsr     r5, expr
        addr    r2, r1          ; R1+=R2
        jsr     r5, answer      ; 0

        mvii    -18000, r1
        mvii    -18000, r2
        mvii    '+', r0
        jsr     r5, expr
        addr    r2, r1          ; R1+=R2
        jsr     r5, answer      ; 29536

        mvii    -18000, r1
        mvii    -28000, r2
        mvii    '-', r0
        jsr     r5, expr
        subr    r2, r1          ; R1-=R2
        jsr     r5, answer      ; 10000

        mvii    18000, r1
        mvii    -18000, r2
        mvii    '-', r0
        jsr     r5, expr
        subr    r2, r1          ; R1-=R2
        jsr     r5, answer      ; -29536

        mvii    -28000, r1
        mvii    -18000, r2
        mvii    '-', r0
        jsr     r5, expr
        subr    r2, r1          ; R1-=R2
        jsr     r5, answer      ; -10000

        mvii    100, r1
        mvii    300, r2
        mvii    '*', r0
        jsr     r5, expr
        jsr     r5, mul16       ; R1*=R2
        jsr     r5, answer      ; 30000

        mvii    200, r1
        mvii    100, r2
        mvii    '*', r0
        jsr     r5, expr
        jsr     r5, mul16       ; R1*=R2
        jsr     r5, answer      ; 20000

        mvii    300, r1
        mvii    -200, r2
        mvii    '*', r0
        jsr     r5, expr
        jsr     r5, mul16       ; R1*=R2
        jsr     r5, answer      ; 5536

        mvii    100, r1
        mvii    -300, r2
        mvii    '*', r0
        jsr     r5, expr
        jsr     r5, mul16       ; R1*=R2
        jsr     r5, answer      ; -30000

        mvii    -200, r1
        mvii    -100, r2
        mvii    '*', r0
        jsr     r5, expr
        jsr     r5, mul16       ; R1*=R2
        jsr     r5, answer      ; 20000

        mvii    30000, r1
        mvii    100, r2
        mvii    '/', r0
        jsr     r5, expr
        jsr     r5, div16       ; R1/=R2
        jsr     r5, answer      ; 300

        mvii    -200, r1
        mvii    100, r2
        mvii    '/', r0
        jsr     r5, expr
        jsr     r5, div16       ; R1/=R2
        jsr     r5, answer      ; -2

        mvii    -30000, r1
        mvii    -200, r2
        mvii    '/', r0
        jsr     r5, expr
        jsr     r5, div16       ; R1/=R2
        jsr     r5, answer      ; 150

        mvii    -30000, r1
        mvii    78, r2
        mvii    '/', r0
        jsr     r5, expr
        jsr     r5, div16       ; R1/=R2
        jsr     r5, answer      ; -384

        mvii    5000, r1
        mvii    4000, r2
        jsr     r5, comp

        mvii    5000, r1
        mvii    5000, r2
        jsr     r5, comp

        mvii    4000, r1
        mvii    5000, r2
        jsr     r5, comp

        mvii    -5000, r1
        mvii    -4000, r2
        jsr     r5, comp

        mvii    -5000, r1
        mvii    -5000, r2
        jsr     r5, comp

        mvii    -4000, r1
        mvii    -5000, r2
        jsr     r5, comp

        mvii    32700, r1
        mvii    32600, r2
        jsr     r5, comp

        mvii    32700, r1
        mvii    32700, r2
        jsr     r5, comp

        mvii    32600, r1
        mvii    32700, r2
        jsr     r5, comp

        mvii    -32700, r1
        mvii    -32600, r2
        jsr     r5, comp

        mvii    -32700, r1
        mvii    -32700, r2
        jsr     r5, comp

        mvii    -32600, r1
        mvii    -32700, r2
        jsr     r5, comp

        mvii    18000, r1
        mvii    -28000, r2
        jsr     r5, comp

        mvii    -28000, r1
        mvii    -28000, r2
        jsr     r5, comp

        mvii    -28000, r1
        mvii    18000, r2
        jsr     r5, comp

        pulr    r7
