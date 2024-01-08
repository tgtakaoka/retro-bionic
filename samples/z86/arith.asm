;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z86c
        option  "reg-alias", "disable"

        include "z8.inc"

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
        jp      init_config

        org     %40
a:      ds      2
b:      ds      2

        org     %1000
stack:  equ     $

init_config:
        srp     #%F0
        setrp   %F0
        ;; Stack is on external memory
        ld      P01M, #P01M_P0ADDR LOR P01M_P1DATA
        ld      P2M, #%FF       ; Port 2 is input
        ld      SPH, #HIGH stack
        ld      SPL, #LOW stack

init_usart:
        srp     #%10
        setrp   %10
        ld      r12, #HIGH USARTC
        ld      r13, #LOW USARTC
        clr     r0
        lde     @rr12, r0
        lde     @rr12, r0
        lde     @rr12, r0       ; safest way to sync mode
        ld      r0, #CMD_IR_bm
        lde     @rr12, r0       ; reset
        nop
        nop
        ld      r0, #ASYNC_MODE
        lde     @rr12, r0       ; async 1stop 8data x16
        nop
        nop
        ld      r0, #RX_EN_TX_EN
        lde     @rr12, r0 ; RTS/DTR, error reset, Rx enable, Tx enable
        ld      r8, #HIGH USARTD
        ld      r9, #LOW USARTD

        call      arith
        halt

putchar:
        push    r0
        incw    rr8
putchar_loop:   
        lde     r0, @rr8
        tm      r0, #ST_TxRDY_bm
        jr      z, putchar_loop
        pop     r0
        decw    rr8
        lde     @rr8, r0
        ret

newline:
        push    r0
        ld      r0, #%0D
        call    putchar
        ld      r0, #%0A
        call    putchar
        pop     r0
        ret

putspace:
        push    r0
        ld      r0, #' '
        call    putchar
        pop     r0
        ret

putflags:
        ld      r1, FLAGS
        tm      r1, #F_CARRY LOR F_ZERO LOR F_SIGN LOR F_OVERFLOW
        jr      nz, putflags_spc
        ret
putflags_spc:
        call    putspace
        ld      FLAGS, r1
        jr      nc, putflags_nc
        ld      r0, #'C'
        call    putchar
putflags_nc:
        ld      FLAGS, r1
        jr      nz, putflags_nz
        ld      r0, #'Z'
        call    putchar
putflags_nz:
        ld      FLAGS, r1
        jr      pl, putflags_pl
        ld      r0, #'S'
        call    putchar
putflags_pl:
        ld      FLAGS, r1
        jr      nov, putflags_nov
        ld      r0, #'V'
        call    putchar
putflags_nov:
        ret

expr:
        push    r0
        ld      r0, a
        ld      r1, a+1
        call    print_int16
        call    putspace
        pop     r0
        call    putchar
        call    putspace
        ld      r0, b
        ld      r1, b+1
        jp      print_int16

answer:
        call    putspace
        ld      r0, #'='
        call    putchar
        call    putspace
        ld      r0, a
        ld      r1, a+1
        call    print_int16
        jp      newline

comp:
        ld      r4, #a
        ld      r5, #b
        call    cmpsi2
        push    FLAGS
        jr      gt, comp_gt
        jr      eq, comp_eq
        jr      lt, comp_lt
        ld      r0, #'?'
        jr      comp_out
comp_gt:
        ld      r0, #'>'
        jr      comp_out
comp_eq:
        ld      r0, #'='
        jr      comp_out
comp_lt:
        ld      r0, #'<'
comp_out:
        call    expr
        pop     FLAGS
        call    putflags
        jp      newline

arith:
        ld      r4, #a
        ld      r5, #b

        ld      a, #0
        ld      a+1, #0
        ld      b, #HIGH -28000
        ld      b+1, #LOW -28000
        ld      r0, #'-'
        call    expr
        call    negsi2
        call    answer          ; 28000

        ld      a, #0
        ld      a+1, #0
        ld      b, #HIGH 28000
        ld      b+1, #LOW 28000
        ld      r0, #'-'
        call    expr
        call    negsi2
        call    answer          ; -28000

        ld      a, #HIGH 18000
        ld      a+1, #LOW 18000
        ld      b, #HIGH 28000
        ld      b+1, #LOW 28000
        ld      r0, #'+'
        call    expr
        call    addsi2
        call    answer          ; -19536

        ld      a, #HIGH 18000
        ld      a+1, #LOW 18000
        ld      b, #HIGH -18000
        ld      b+1, #LOW -18000
        ld      r0, #'+'
        call    expr
        call    addsi2
        call    answer          ; 0

        ld      a, #HIGH -28000
        ld      a+1, #LOW -28000
        ld      r0, #'+'
        call    expr
        call    addsi2
        call    answer          ; 29536

        ld      a, #HIGH 18000
        ld      a+1, #LOW 18000
        ld      b, #HIGH -28000
        ld      b+1, #LOW -28000
        ld      r0, #'-'
        call    expr
        call    subsi2
        call    answer          ; -19536

        ld      a, #HIGH 18000
        ld      a+1, #LOW 18000
        ld      b, #HIGH -18000
        ld      b+1, #LOW -18000
        ld      r0, #'-'
        call    expr
        call    subsi2
        call    answer          ; 29536

        ld      a, #HIGH -28000
        ld      a+1, #LOW -28000
        ld      r0, #'-'
        call    expr
        call    subsi2
        call    answer          ; -10000

        ld      a, #HIGH 300
        ld      a+1, #LOW 300
        ld      b, #HIGH -200
        ld      b+1, #LOW -200
        ld      r0, #'*'
        call    expr
        call    mulsi2
        call    answer          ; 5536

        ld      a, #HIGH 100
        ld      a+1, #LOW 100
        ld      b, #HIGH -300
        ld      b+1, #LOW -300
        ld      r0, #'*'
        call    expr
        call    mulsi2
        call    answer          ; -30000

        ld      a, #HIGH -200
        ld      a+1, #LOW -200
        ld      b, #HIGH -100
        ld      b+1, #LOW -100
        ld      r0, #'*'
        call    expr
        call    mulsi2
        call    answer          ; 20000

        ld      a, #HIGH -200
        ld      a+1, #LOW -200
        ld      b, #HIGH 100
        ld      b+1, #LOW 100
        ld      r0, #'/'
        call    expr
        call    divsi2
        call    answer          ; -2

        ld      a, #HIGH 30000
        ld      a+1, #LOW 30000
        ld      r0, #'/'
        call    expr
        call    divsi2
        call    answer          ; 30

        ld      a, #HIGH -30000
        ld      a+1, #LOW -30000
        ld      b, #HIGH -200
        ld      b+1, #LOW -200
        ld      r0, #'/'
        call    expr
        call    divsi2
        call    answer          ; 150

        ld      a, #HIGH -30000
        ld      a+1, #LOW -30000
        ld      b, #HIGH 78
        ld      b+1, #LOW 78
        ld      r0, #'/'
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
