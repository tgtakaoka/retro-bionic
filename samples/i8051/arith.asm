;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8051
        include "i8051.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     0FFF0H
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

;;; External data memory
        org     2000H
print_uint16_buf:
        ds      8

;;; Internal data memory
        org     BASE_MEMORY
vA:     ds      2
vB:     ds      2
stack:  equ     $

        org     ORG_RESET
        ljmp    init
        org     ORG_IE0
        ljmp    init
        org     ORG_TF0
        ljmp    init
        org     ORG_IE1
        ljmp    init
        org     ORG_TF1
        ljmp    init
        org     ORG_RITI
        ljmp    init

init:
        mov     SP, #stack-1
init_uart:
        mov     DPTR, #USARTC
        clr     A
        movx    @DPTR, A
        movx    @DPTR, A
        movx    @DPTR, A        ; safest way to sync mode
        mov     A, #CMD_IR_bm   ; reset
        movx    @DPTR, A
        nop
        nop
        mov     A, #ASYNC_MODE
        movx    @DPTR, A
        nop
        nop
        mov     A, #RX_EN_TX_EN
        movx    @DPTR, A

        ljmp    arith

;;; Get character
;;; @return A received character (PSW.C=1)
;;; @return PSW.C=0 no character
getchar:
        push    DPH
        push    DPL
        mov     DPTR, #USARTS
        movx    A, @DPTR
        clr     C
        jnb     ACC.ST_RxRDY_bp, getchar_exit
        dec     DPL
        movx    A, @DPTR
        setb    C
getchar_exit:
        pop     DPL
        pop     DPH
        ret

putspace:
        mov     A, #' '

;;; Put character
;;; @param A Sending character
;;; @clobber A
putchar:
        push    DPH
        push    DPL             ; save DPTR
        mov     DPTR, #USARTS
        push    ACC             ; save character
putchar_loop:
        movx    A, @DPTR
        jnb     ACC.ST_TxRDY_bp, putchar_loop
        pop     ACC             ; restore character
        dec     DPL
        movx    @DPTR, A        ; send character
        pop     DPL
        pop     DPH             ; restore DPTR
        ret

;;; Set v1/v2 to vA/vB
;;; @param R3:R2 v1
;;; @param R5:R4 v2
;;; @return R2 &vA
;;; @return R3 &vB
;;; @clobber R0
set_vAvB:
        push    ACC             ; save A
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
        pop     ACC
        mov     R2, #vA
        mov     R3, #vB
        ret

;;; Print "v1 op v2"
;;; @param R3:R2 v1
;;; @param R5:R4 v2
;;; @param A op
;;; @return R2 &vA
;;; @return R3 &vB
;;; @clobber A
expr:
        acall   set_vAvB
expr_out:
        push    ACC             ; save op
        mov     R0, #vA
        acall   print_int16     ; print v1
        acall   putspace
        pop     ACC             ; restore op
        acall   putchar         ; print op
        acall   putspace
        mov     R0, #vB
        acall   print_int16
        mov     R2, #vA
        mov     R3, #vB
        ret

;;; Print " = val\n"
;;; @param vA val
;;; @clobber A
answer:
        acall   putspace
        mov     A, #'='
        acall   putchar
        acall   putspace
        mov     R0, #vA
        acall   print_int16
newline:
        mov     A, #0DH
        acall   putchar
        mov     A, #0AH
        sjmp    putchar

;;; Print "v1 rel v2\n"
;;; @param R2:R3 v1
;;; @param R4:R5 v2
comp:
        acall   set_vAvB
        acall   cmpsi2
        jz      comp_eq
        jb      ACC.7, comp_lt
comp_gt:
        mov     A, #'>'
comp_out:
        acall   expr_out
        sjmp    newline
comp_eq:
        mov     A, #'='
        sjmp    comp_out
comp_lt:
        mov     A, #'<'
        sjmp    comp_out

arith:
        mov     R2, #LOW(0)
        mov     R3, #HIGH(0)
        mov     R4, #LOW(-28000)
        mov     R5, #HIGH(-28000)
        mov     A, #'-'
        acall   expr
        acall   negsi2
        acall   answer          ; 28000

        mov     R2, #LOW(0)
        mov     R3, #HIGH(0)
        mov     R4, #LOW(28000)
        mov     R5, #HIGH(28000)
        mov     A, #'-'
        acall   expr
        acall   negsi2
        acall   answer          ; -28000

        mov     R2, #LOW(18000)
        mov     R3, #HIGH(18000)
        mov     R4, #LOW(28000)
        mov     R5, #HIGH(28000)
        mov     A, #'+'
        acall   expr
        acall   addsi2
        acall   answer          ; -19536

        mov     R2, #LOW(18000)
        mov     R3, #HIGH(18000)
        mov     R4, #LOW(-18000)
        mov     R5, #HIGH(-18000)
        mov     A, #'+'
        acall   expr
        acall   addsi2
        acall   answer          ; 0

        mov     R2, #LOW(-28000)
        mov     R3, #HIGH(-28000)
        mov     R4, #LOW(-18000)
        mov     R5, #HIGH(-18000)
        mov     A, #'+'
        acall   expr
        acall   addsi2
        acall   answer          ; 19536

        mov     R2, #LOW(-18000)
        mov     R3, #HIGH(-18000)
        mov     R4, #LOW(-28000)
        mov     R5, #HIGH(-28000)
        mov     A, #'-'
        acall   expr
        acall   subsi2
        acall   answer          ; 10000

        mov     R2, #LOW(18000)
        mov     R3, #HIGH(18000)
        mov     R4, #LOW(-18000)
        mov     R5, #HIGH(-18000)
        mov     A, #'-'
        acall   expr
        acall   subsi2
        acall   answer          ; -29536

        mov     R2, #LOW(-28000)
        mov     R3, #HIGH(-28000)
        mov     R4, #LOW(-18000)
        mov     R5, #HIGH(-18000)
        mov     A, #'-'
        acall   expr
        acall   subsi2
        acall   answer          ; -10000

        mov     R2, #LOW(300)
        mov     R3, #HIGH(300)
        mov     R4, #LOW(-200)
        mov     R5, #HIGH(-200)
        mov     A, #'*'
        acall   expr
        acall   mulsi2
        acall   answer          ; 5536

        mov     R2, #LOW(100)
        mov     R3, #HIGH(100)
        mov     R4, #LOW(-300)
        mov     R5, #HIGH(-300)
        mov     A, #'*'
        acall   expr
        acall   mulsi2
        acall   answer          ; -30000

        mov     R2, #LOW(-200)
        mov     R3, #HIGH(-200)
        mov     R4, #LOW(-100)
        mov     R5, #HIGH(-100)
        mov     A, #'*'
        acall   expr
        acall   mulsi2
        acall   answer          ; 20000

        mov     R2, #LOW(-200)
        mov     R3, #HIGH(-200)
        mov     R4, #LOW(100)
        mov     R5, #HIGH(100)
        mov     A, #'/'
        acall   expr
        acall   divsi2
        acall   answer          ; -2

        mov     R2, #LOW(30000)
        mov     R3, #HIGH(30000)
        mov     R4, #LOW(100)
        mov     R5, #HIGH(100)
        mov     A, #'/'
        acall   expr
        acall   divsi2
        acall   answer          ; 30

        mov     R2, #LOW(-30000)
        mov     R3, #HIGH(-30000)
        mov     R4, #LOW(-200)
        mov     R5, #HIGH(-200)
        mov     A, #'/'
        acall   expr
        acall   divsi2
        acall   answer          ; 150

        mov     R2, #LOW(-30000)
        mov     R3, #HIGH(-30000)
        mov     R4, #LOW(78)
        mov     R5, #HIGH(78)
        mov     A, #'/'
        acall   expr
        acall   divsi2
        acall   answer          ; -384

        mov     R2, #LOW(5000)
        mov     R3, #HIGH(5000)
        mov     R4, #LOW(4000)
        mov     R5, #HIGH(4000)
        acall   comp

        mov     R2, #LOW(5000)
        mov     R3, #HIGH(5000)
        mov     R4, #LOW(5000)
        mov     R5, #HIGH(5000)
        acall   comp

        mov     R2, #LOW(4000)
        mov     R3, #HIGH(4000)
        mov     R4, #LOW(5000)
        mov     R5, #HIGH(5000)
        acall   comp

        mov     R2, #LOW(-5000)
        mov     R3, #HIGH(-5000)
        mov     R4, #LOW(-4000)
        mov     R5, #HIGH(-4000)
        acall   comp

        mov     R2, #LOW(-5000)
        mov     R3, #HIGH(-5000)
        mov     R4, #LOW(-5000)
        mov     R5, #HIGH(-5000)
        acall   comp

        mov     R2, #LOW(-4000)
        mov     R3, #HIGH(-4000)
        mov     R4, #LOW(-5000)
        mov     R5, #HIGH(-5000)
        acall   comp

        mov     R2, #LOW(32700)
        mov     R3, #HIGH(32700)
        mov     R4, #LOW(32600)
        mov     R5, #HIGH(32600)
        acall   comp

        mov     R2, #LOW(32700)
        mov     R3, #HIGH(32700)
        mov     R4, #LOW(32700)
        mov     R5, #HIGH(32700)
        acall   comp

        mov     R2, #LOW(32600)
        mov     R3, #HIGH(32600)
        mov     R4, #LOW(32700)
        mov     R5, #HIGH(32700)
        acall   comp

        mov     R2, #LOW(-32700)
        mov     R3, #HIGH(-32700)
        mov     R4, #LOW(-32600)
        mov     R5, #HIGH(-32600)
        acall   comp

        mov     R2, #LOW(-32700)
        mov     R3, #HIGH(-32700)
        mov     R4, #LOW(-32700)
        mov     R5, #HIGH(-32700)
        acall   comp

        mov     R2, #LOW(-32600)
        mov     R3, #HIGH(-32600)
        mov     R4, #LOW(-32700)
        mov     R5, #HIGH(-32700)
        acall   comp

        mov     R2, #LOW(18000)
        mov     R3, #HIGH(18000)
        mov     R4, #LOW(-18000)
        mov     R5, #HIGH(-18000)
        acall   comp

        mov     R2, #LOW(18000)
        mov     R3, #HIGH(18000)
        mov     R4, #LOW(18000)
        mov     R5, #HIGH(18000)
        acall   comp

        mov     R2, #LOW(-18000)
        mov     R3, #HIGH(-18000)
        mov     R4, #LOW(18000)
        mov     R5, #HIGH(18000)
        acall   comp

        acall   newline
        db      0A5H            ; halt to system

        include "arith.inc"
