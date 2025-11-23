;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z88.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     %FF00
USARTD: equ     0               ; Data register
USARTS: equ     1               ; Status register
USARTC: equ     1               ; Control register
        include "../z86/i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc LOR MODE_LEN8_gc LOR MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm LOR CMD_TxEN_bm

        org     %1000
stack:  equ     $

        org     ORG_RESET
        setrp   %C0
init_config:
        ld      EMT, #EMT_STACK_DM ; stack is on external data memory
        ld      P0M, #P0M_ADRH     ; Port 0 is address bus high
        ld      PM, #PM_P1BUS | PM_DM ; Port 1 is data bus and address bus low
        ldw     SPH, #stack

init_usart:
        ldw     RR12, #USART
        clr     R0
        lde     USARTC(RR12), R0
        lde     USARTC(RR12), R0
        lde     USARTC(RR12), R0 ; safest way to sync mode
        ld      R0, #CMD_IR_bm
        lde     USARTC(RR12), R0 ; reset
        nop
        nop
        ld      R0, #MODE_STOP1_gc LOR MODE_LEN8_gc LOR MODE_BAUD_X16
        lde     USARTC(RR12), R0 ; async 1stop 8data x16
        nop
        nop
        ld      R0, #CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm LOR CMD_TxEN_bm
        lde     USARTC(RR12), R0 ; RTS/DTR, error reset, Rx enable, Tx enable

        call    arith
        call    newline
        wfi                     ; halt to system

putchar:
        push    R1
        ldw     RR12, #USART
putchar_loop:
        lde     R1, USARTS(RR12)
        tm      R1, #ST_TxRDY_bm
        jr      z, putchar_loop
        lde     USARTD(RR12), R0
        cp      R0, #%0D
        jr      nz, putchar_end
        ld      R0, #%0A
        jr      putchar_loop
putchar_end:
        pop     R1
        ret

newline:
        push    R0
        ld      R0, #%0D
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
        ldw     RR0, a
        call    print_int16
        call    putspace
        pop     R0
        call    putchar
        call    putspace
        ldw     RR0, b
        jp      print_int16

answer:
        call    putspace
        ld      R0, #'='
        call    putchar
        call    putspace
        ldw     RR0, a
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

        org     %10
a:      ds      2
b:      ds      2

        org     %1000

arith:
        ld      R4, #a
        ld      R5, #b

        ldw     a, #18000
        ldw     b, #28000
        ld      R0, #'+'
        call    expr
        call    addsi2
        call    answer

        ldw     a, #18000
        ldw     b, #-18000
        ld      R0, #'+'
        call    expr
        call    addsi2
        call    answer

        ldw     a, #-28000
        ld      R0, #'+'
        call    expr
        call    addsi2
        call    answer

        ldw     a, #18000
        ldw     b, #-28000
        ld      R0, #'-'
        call    expr
        call    subsi2
        call    answer

        ldw     a, #18000
        ldw     b, #18000
        ld      R0, #'-'
        call    expr
        call    subsi2
        call    answer

        ldw     a, #-28000
        ld      R0, #'-'
        call    expr
        call    subsi2
        call    answer

        ldw     a, #300
        ldw     b, #-200
        ld      R0, #'*'
        call    expr
        call    mulsi2
        call    answer

        ldw     a, #100
        ldw     b, #-300
        ld      R0, #'*'
        call    expr
        call    mulsi2
        call    answer

        ldw     a, #-200
        ldw     b, #100
        ld      R0, #'*'
        call    expr
        call    mulsi2
        call    answer

        ldw     b, #100
        ld      R0, #'/'
        call    expr
        call    divsi2
        call    answer

        ldw     a, #30000
        ldw     b, #0
        ld      R0, #'/'
        call    expr
        call    divsi2
        call    answer

        ldw     a, #-30000
        ldw     b, #78
        ld      R0, #'/'
        call    expr
        call    divsi2
        call    answer

        ldw     a, #5000
        ldw     b, #4000
        call    comp

        ldw     b, #5000
        call    comp

        ldw     a, #4000
        call    comp

        ldw     a, #-5000
        ldw     b, #-4000
        call    comp

        ldw     b, #-5000
        call    comp

        ldw     a, #-4000
        call    comp

        ldw     a, #32700
        ldw     b, #32600
        call    comp

        ldw     b, #32700
        call    comp

        ldw     a, #32600
        call    comp

        ldw     a, #-32700
        ldw     b, #-32600
        call    comp

        ldw     b, #-32700
        call    comp

        ldw     a, #-32600
        call    comp

        ldw     a, #18000
        ldw     b, #-28000
        call    comp

        ldw     b, #18000
        call    comp

        ldw     a, #-28000
        call    comp

        ret

        include "arith.inc"

        end
