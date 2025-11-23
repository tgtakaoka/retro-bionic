;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z86.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     %FF00
USARTD: equ     USART+0         ; Data register
USARTS: equ     USART+1         ; Status register
USARTC: equ     USART+1         ; Control register
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc LOR MODE_LEN8_gc LOR MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm LOR CMD_TxEN_bm

        org     ORG_RESET
        setrp   -1
        jp      init_config

        org     %40
a:      ds      2
b:      ds      2

        org     %1000
stack:  equ     $

init_config:
        ;; Stack is on external memory
        ld      P01M, #P01M_P0ADDR LOR P01M_P1DATA
        ld      P2M, #%FF       ; Port 2 is input
        ld      SPH, #HIGH stack
        ld      SPL, #LOW stack

init_usart:
        srp     #%10
        setrp   %10
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
        ld      R0, #RX_EN_TX_EN
        lde     @RR12, R0 ; RTS/DTR, error reset, Rx enable, Tx enable
        ld      R8, #HIGH USARTD
        ld      R9, #LOW USARTD

        call      arith
        halt

putchar:
        push    R0
        incw    RR8
putchar_loop:   
        lde     R0, @RR8
        tm      R0, #ST_TxRDY_bm
        jr      z, putchar_loop
        pop     R0
        decw    RR8
        lde     @RR8, R0
        ret

newline:
        push    R0
        ld      R0, #%0D
        call    putchar
        ld      R0, #%0A
        call    putchar
        pop     R0
        ret

putspace:
        push    R0
        ld      R0, #' '
        call    putchar
        pop     R0
        ret

putflags:
        ld      R1, FLAGS
        tm      R1, #F_CARRY LOR F_ZERO LOR F_SIGN LOR F_OVERFLOW
        jr      nz, putflags_spc
        ret
putflags_spc:
        call    putspace
        ld      FLAGS, R1
        jr      nc, putflags_nc
        ld      R0, #'C'
        call    putchar
putflags_nc:
        ld      FLAGS, R1
        jr      nz, putflags_nz
        ld      R0, #'Z'
        call    putchar
putflags_nz:
        ld      FLAGS, R1
        jr      pl, putflags_pl
        ld      R0, #'S'
        call    putchar
putflags_pl:
        ld      FLAGS, R1
        jr      nov, putflags_nov
        ld      R0, #'V'
        call    putchar
putflags_nov:
        ret

expr:
        push    R0
        ld      R0, a
        ld      R1, a+1
        call    print_int16
        call    putspace
        pop     R0
        call    putchar
        call    putspace
        ld      R0, b
        ld      R1, b+1
        jp      print_int16

answer:
        call    putspace
        ld      R0, #'='
        call    putchar
        call    putspace
        ld      R0, a
        ld      R1, a+1
        call    print_int16
        jp      newline

comp:
        ld      R4, #a
        ld      R5, #b
        call    cmpsi2
        push    FLAGS
        jr      gt, comp_gt
        jr      eq, comp_eq
        jr      lt, comp_lt
        ld      R0, #'?'
        jr      comp_out
comp_gt:
        ld      R0, #'>'
        jr      comp_out
comp_eq:
        ld      R0, #'='
        jr      comp_out
comp_lt:
        ld      R0, #'<'
comp_out:
        call    expr
        pop     FLAGS
        call    putflags
        jp      newline

arith:
        ld      R4, #a
        ld      R5, #b

        ld      a, #0
        ld      a+1, #0
        ld      b, #HIGH -28000
        ld      b+1, #LOW -28000
        ld      R0, #'-'
        call    expr
        call    negsi2
        call    answer          ; 28000

        ld      a, #0
        ld      a+1, #0
        ld      b, #HIGH 28000
        ld      b+1, #LOW 28000
        ld      R0, #'-'
        call    expr
        call    negsi2
        call    answer          ; -28000

        ld      a, #HIGH 18000
        ld      a+1, #LOW 18000
        ld      b, #HIGH 28000
        ld      b+1, #LOW 28000
        ld      R0, #'+'
        call    expr
        call    addsi2
        call    answer          ; -19536

        ld      a, #HIGH 18000
        ld      a+1, #LOW 18000
        ld      b, #HIGH -18000
        ld      b+1, #LOW -18000
        ld      R0, #'+'
        call    expr
        call    addsi2
        call    answer          ; 0

        ld      a, #HIGH -28000
        ld      a+1, #LOW -28000
        ld      R0, #'+'
        call    expr
        call    addsi2
        call    answer          ; 29536

        ld      a, #HIGH 18000
        ld      a+1, #LOW 18000
        ld      b, #HIGH -28000
        ld      b+1, #LOW -28000
        ld      R0, #'-'
        call    expr
        call    subsi2
        call    answer          ; -19536

        ld      a, #HIGH 18000
        ld      a+1, #LOW 18000
        ld      b, #HIGH -18000
        ld      b+1, #LOW -18000
        ld      R0, #'-'
        call    expr
        call    subsi2
        call    answer          ; 29536

        ld      a, #HIGH -28000
        ld      a+1, #LOW -28000
        ld      R0, #'-'
        call    expr
        call    subsi2
        call    answer          ; -10000

        ld      a, #HIGH 300
        ld      a+1, #LOW 300
        ld      b, #HIGH -200
        ld      b+1, #LOW -200
        ld      R0, #'*'
        call    expr
        call    mulsi2
        call    answer          ; 5536

        ld      a, #HIGH 100
        ld      a+1, #LOW 100
        ld      b, #HIGH -300
        ld      b+1, #LOW -300
        ld      R0, #'*'
        call    expr
        call    mulsi2
        call    answer          ; -30000

        ld      a, #HIGH -200
        ld      a+1, #LOW -200
        ld      b, #HIGH -100
        ld      b+1, #LOW -100
        ld      R0, #'*'
        call    expr
        call    mulsi2
        call    answer          ; 20000

        ld      a, #HIGH -200
        ld      a+1, #LOW -200
        ld      b, #HIGH 100
        ld      b+1, #LOW 100
        ld      R0, #'/'
        call    expr
        call    divsi2
        call    answer          ; -2

        ld      a, #HIGH 30000
        ld      a+1, #LOW 30000
        ld      R0, #'/'
        call    expr
        call    divsi2
        call    answer          ; 30

        ld      a, #HIGH -30000
        ld      a+1, #LOW -30000
        ld      b, #HIGH -200
        ld      b+1, #LOW -200
        ld      R0, #'/'
        call    expr
        call    divsi2
        call    answer          ; 150

        ld      a, #HIGH -30000
        ld      a+1, #LOW -30000
        ld      b, #HIGH 78
        ld      b+1, #LOW 78
        ld      R0, #'/'
        call    expr
        call    divsi2
        call    answer          ; -384

        ld      a, #HIGH 5000
        ld      a+1, #LOW 5000
        ld      b, #HIGH 4000
        ld      b+1, #LOW 4000
        call    comp

        ld      b, #HIGH 5000
        ld      b+1, #LOW 5000
        call    comp

        ld      a, #HIGH 4000
        ld      a+1, #LOW 4000
        call    comp

        ld      a, #HIGH -5000
        ld      a+1, #LOW -5000
        ld      b, #HIGH -4000
        ld      b+1, #LOW -4000
        call    comp

        ld      b, #HIGH -5000
        ld      b+1, #LOW -5000
        call    comp

        ld      a, #HIGH -4000
        ld      a+1, #LOW -4000
        call    comp

        ld      a, #HIGH 32700
        ld      a+1, #LOW 32700
        ld      b, #HIGH 32600
        ld      b+1, #LOW 32600
        call    comp

        ld      b, #HIGH 32700
        ld      b+1, #LOW 32700
        call    comp

        ld      a, #HIGH 32600
        ld      a+1, #LOW 32600
        call    comp

        ld      a, #HIGH -32700
        ld      a+1, #LOW -32700
        ld      b, #HIGH -32600
        ld      b+1, #LOW -32600
        call    comp

        ld      b, #HIGH -32700
        ld      b+1, #LOW -32700
        call    comp

        ld      a, #HIGH -32600
        ld      a+1, #LOW -32600
        call    comp

        ld      a, #HIGH 18000
        ld      a+1, #LOW 18000
        ld      b, #HIGH -28000
        ld      b+1, #LOW -28000
        call    comp

        ld      b, #HIGH 18000
        ld      b+1, #LOW 18000
        call    comp

        ld      a, #HIGH -28000
        ld      a+1, #LOW -28000
        call    comp

        ret

        include "arith.inc"

        end
