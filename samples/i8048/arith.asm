;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8048
        include "i8048.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     0FCH
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

;;; External data memory
        org     0
print_uint16_buf:
        ds      8
        org     USART
;;;  Software stack; pre-decrement, post-increment pointed by R1 on
;;;  external data memory
stack:          equ     $

;;; Internal data memory
        org     BASE_MEMORY
vA:     ds      2
vB:     ds      2

        org     ORG_RESET
        jmp     init
        org     ORG_INT
        jmp     init
init:
        mov     R1, #stack
        mov     R0, #USARTC
        clr     A
        movx    @R0, A
        movx    @R0, A
        movx    @R0, A          ; safest way to sync mode
        mov     A, #CMD_IR_bm   ; reset
        movx    @R0, A
        nop
        nop
        mov     A, #ASYNC_MODE
        movx    @R0, A
        nop
        nop
        mov     A, #RX_EN_TX_EN
        movx    @R0, A

        jmp     arith

;;; Get character
;;; @return A received character (PSW.C=1)
;;; @return PSW.C=0 no character
getchar:
        mov     A, R0
        dec     R1
        movx    @R1, A          ; save R0
        mov     R0, #USARTS
        movx    A, @R0
        clr     C
        cpl     A
        jb      ST_RxRDY_bp, getchar_exit
        dec     R0
        movx    A, @R0
        cpl     C
getchar_exit:
        xch     A, R0           ; save character
        movx    A, @R1          ; restore R0
        inc     R1
        xch     A, R0           ; restore character
        ret

putspace:
        mov     A, #' '

;;; Put character
;;; @param A Sending character
;;; @clobber A
putchar:
        xch     A, R0           ; save character
        dec     R1
        movx    @R1, A          ; save R0
        mov     A, #USARTS
        xch     A, R0           ; restore character
        xch     A, R2           ; save character
        dec     R1
        movx    @R1, A          ; save R2
putchar_loop:
        movx    A, @R0
        cpl     A
        jb      ST_TxRDY_bp, putchar_loop
        mov     A, R2           ; restore character
        dec     R0
        movx    @R0, A          ; send character
        movx    A, @R1
        inc     R1
        mov     R2, A           ; restore R2
        movx    A, @R1
        inc     R1
        mov     R0, A           ; restore R0
        ret

;;; Print "v1 op v2"
;;; @param R3:R2 v1
;;; @param R5:R4 v2
;;; @param A op
;;; @return R2 &vA
;;; @return R3 &vB
;;; @clobber A
expr:
        dec     R1
        movx    @R1, A          ; save op
        mov     R0, #vA
        mov     A, R2
        mov     @R0, A
        mov     A, R3
        inc     R0
        mov     @R0, A          ; vA=v1
        mov     A, R4
        inc     R0
        mov     @R0, A
        mov     A, R5
        inc     R0
        mov     @R0, A          ; vB=v2
expr_out:
        mov     R0, #vA
        call    print_int16     ; print v1
        call    putspace
        movx    A, @R1          ; restore op
        inc     R1
        call    putchar         ; print op
        call    putspace
        mov     R0, #vB
        call    print_int16
        mov     R2, #vA
        mov     R3, #vB
        ret

;;; Print " = val\n"
;;; @param vA val
;;; @clobber A
answer:
        call    putspace
        mov     A, #'='
        call    putchar
        call    putspace
        mov     R0, #vA
        call    print_int16
newline:
        mov     A, #0DH
        call    putchar
        mov     A, #0AH
        jmp     putchar

;;; Print "v1 rel v2\n"
;;; @param R2:R3 v1
;;; @param R4:R5 v2
comp:
        mov     A, R2
        mov     R0, #vA
        mov     @R0, A
        mov     A, R3
        inc     R0
        mov     @R0, A          ; vA=v1
        mov     A, R4
        inc     R0
        mov     @R0, A
        mov     A, R5
        inc     R0
        mov     @R0, A          ; vB=v2
        mov     R2, #vA
        mov     R3, #vB
        call    cmpsi2
        jz      comp_eq
        jb      7, comp_lt
comp_gt:
        mov     A, #'>'
comp_out:
        dec     R1
        movx    @R1, A          ; save rel
        call    expr_out
        jmp     newline
comp_eq:
        mov     A, #'='
        jmp     comp_out
comp_lt:
        mov     A, #'<'
        jmp     comp_out

arith:
        mov     R2, #LOW(0)
        mov     R3, #HIGH(0)
        mov     R4, #LOW(-28000)
        mov     R5, #HIGH(-28000)
        mov     A, #'-'
        call    expr
        call    negsi2
        call    answer          ; 28000

        mov     R2, #LOW(0)
        mov     R3, #HIGH(0)
        mov     R4, #LOW(28000)
        mov     R5, #HIGH(28000)
        mov     A, #'-'
        call    expr
        call    negsi2
        call    answer          ; -28000

        mov     R2, #LOW(18000)
        mov     R3, #HIGH(18000)
        mov     R4, #LOW(28000)
        mov     R5, #HIGH(28000)
        mov     A, #'+'
        call    expr
        call    addsi2
        call    answer          ; -19536

        mov     R2, #LOW(18000)
        mov     R3, #HIGH(18000)
        mov     R4, #LOW(-18000)
        mov     R5, #HIGH(-18000)
        mov     A, #'+'
        call    expr
        call    addsi2
        call    answer          ; 0

        mov     R2, #LOW(-28000)
        mov     R3, #HIGH(-28000)
        mov     R4, #LOW(-18000)
        mov     R5, #HIGH(-18000)
        mov     A, #'+'
        call    expr
        call    addsi2
        call    answer          ; 19536

        mov     R2, #LOW(-18000)
        mov     R3, #HIGH(-18000)
        mov     R4, #LOW(-28000)
        mov     R5, #HIGH(-28000)
        mov     A, #'-'
        call    expr
        call    subsi2
        call    answer          ; 10000

        mov     R2, #LOW(18000)
        mov     R3, #HIGH(18000)
        mov     R4, #LOW(-18000)
        mov     R5, #HIGH(-18000)
        mov     A, #'-'
        call    expr
        call    subsi2
        call    answer          ; -29536

        mov     R2, #LOW(-28000)
        mov     R3, #HIGH(-28000)
        mov     R4, #LOW(-18000)
        mov     R5, #HIGH(-18000)
        mov     A, #'-'
        call    expr
        call    subsi2
        call    answer          ; -10000

        mov     R2, #LOW(300)
        mov     R3, #HIGH(300)
        mov     R4, #LOW(-200)
        mov     R5, #HIGH(-200)
        mov     A, #'*'
        call    expr
        call    mulsi2
        call    answer          ; 5536

        mov     R2, #LOW(100)
        mov     R3, #HIGH(100)
        mov     R4, #LOW(-300)
        mov     R5, #HIGH(-300)
        mov     A, #'*'
        call    expr
        call    mulsi2
        call    answer          ; -30000

        mov     R2, #LOW(-200)
        mov     R3, #HIGH(-200)
        mov     R4, #LOW(-100)
        mov     R5, #HIGH(-100)
        mov     A, #'*'
        call    expr
        call    mulsi2
        call    answer          ; 20000

        mov     R2, #LOW(-200)
        mov     R3, #HIGH(-200)
        mov     R4, #LOW(100)
        mov     R5, #HIGH(100)
        mov     A, #'/'
        call    expr
        call    divsi2
        call    answer          ; -2

        mov     R2, #LOW(30000)
        mov     R3, #HIGH(30000)
        mov     R4, #LOW(100)
        mov     R5, #HIGH(100)
        mov     A, #'/'
        call    expr
        call    divsi2
        call    answer          ; 30

        mov     R2, #LOW(-30000)
        mov     R3, #HIGH(-30000)
        mov     R4, #LOW(-200)
        mov     R5, #HIGH(-200)
        mov     A, #'/'
        call    expr
        call    divsi2
        call    answer          ; 150

        mov     R2, #LOW(-30000)
        mov     R3, #HIGH(-30000)
        mov     R4, #LOW(78)
        mov     R5, #HIGH(78)
        mov     A, #'/'
        call    expr
        call    divsi2
        call    answer          ; -384

        mov     R2, #LOW(5000)
        mov     R3, #HIGH(5000)
        mov     R4, #LOW(4000)
        mov     R5, #HIGH(4000)
        call    comp

        mov     R2, #LOW(5000)
        mov     R3, #HIGH(5000)
        mov     R4, #LOW(5000)
        mov     R5, #HIGH(5000)
        call    comp

        mov     R2, #LOW(4000)
        mov     R3, #HIGH(4000)
        mov     R4, #LOW(5000)
        mov     R5, #HIGH(5000)
        call    comp

        mov     R2, #LOW(-5000)
        mov     R3, #HIGH(-5000)
        mov     R4, #LOW(-4000)
        mov     R5, #HIGH(-4000)
        call    comp

        mov     R2, #LOW(-5000)
        mov     R3, #HIGH(-5000)
        mov     R4, #LOW(-5000)
        mov     R5, #HIGH(-5000)
        call    comp

        mov     R2, #LOW(-4000)
        mov     R3, #HIGH(-4000)
        mov     R4, #LOW(-5000)
        mov     R5, #HIGH(-5000)
        call    comp

        mov     R2, #LOW(32700)
        mov     R3, #HIGH(32700)
        mov     R4, #LOW(32600)
        mov     R5, #HIGH(32600)
        call    comp

        mov     R2, #LOW(32700)
        mov     R3, #HIGH(32700)
        mov     R4, #LOW(32700)
        mov     R5, #HIGH(32700)
        call    comp

        mov     R2, #LOW(32600)
        mov     R3, #HIGH(32600)
        mov     R4, #LOW(32700)
        mov     R5, #HIGH(32700)
        call    comp

        mov     R2, #LOW(-32700)
        mov     R3, #HIGH(-32700)
        mov     R4, #LOW(-32600)
        mov     R5, #HIGH(-32600)
        call    comp

        mov     R2, #LOW(-32700)
        mov     R3, #HIGH(-32700)
        mov     R4, #LOW(-32700)
        mov     R5, #HIGH(-32700)
        call    comp

        mov     R2, #LOW(-32600)
        mov     R3, #HIGH(-32600)
        mov     R4, #LOW(-32700)
        mov     R5, #HIGH(-32700)
        call    comp

        mov     R2, #LOW(18000)
        mov     R3, #HIGH(18000)
        mov     R4, #LOW(-18000)
        mov     R5, #HIGH(-18000)
        call    comp

        mov     R2, #LOW(18000)
        mov     R3, #HIGH(18000)
        mov     R4, #LOW(18000)
        mov     R5, #HIGH(18000)
        call    comp

        mov     R2, #LOW(-18000)
        mov     R3, #HIGH(-18000)
        mov     R4, #LOW(18000)
        mov     R5, #HIGH(18000)
        call    comp

        call    newline
        db      01H             ; halt to system

        include "arith.inc"
