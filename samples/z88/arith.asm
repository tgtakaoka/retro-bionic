;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z88
        option  "reg-alias", "disable"
        option  "optimize-index", "enable"

        include "z88.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     %FF00
USARTD: equ     0               ; Data register
USARTS: equ     1               ; Status register
USARTC: equ     1               ; Control register
        include "i8251.inc"
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
        ldw     rr12, #USART
        clr     r0
        lde     USARTC(rr12), r0
        lde     USARTC(rr12), r0
        lde     USARTC(rr12), r0 ; safest way to sync mode
        ld      r0, #CMD_IR_bm
        lde     USARTC(rr12), r0 ; reset
        nop
        nop
        ld      r0, #MODE_STOP1_gc LOR MODE_LEN8_gc LOR MODE_BAUD_X16
        lde     USARTC(rr12), r0 ; async 1stop 8data x16
        nop
        nop
        ld      r0, #CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm LOR CMD_TxEN_bm
        lde     USARTC(rr12), r0 ; RTS/DTR, error reset, Rx enable, Tx enable

        call    arith
        wfi                     ; halt to system
        jr      $

putchar:
        push    r1
        ldw     rr12, #USART
putchar_loop:
        lde     r1, USARTS(rr12)
        tm      r1, #ST_TxRDY_bm
        jr      z, putchar_loop
        lde     USARTD(rr12), r0
        cp      r0, #%0D
        jr      nz, putchar_end
        ld      r0, #%0A
        jr      putchar_loop
putchar_end:
        pop     r1
        ret

newline:
        push    r0
        ld      r0, #%0D
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
        ldw     rr0, a
        call    print_int16
        call    putspace
        pop     r0
        call    putchar
        call    putspace
        ldw     rr0, b
        jp      print_int16

answer:
        call    putspace
        ld      r0, #'='
        call    putchar
        call    putspace
        ldw     rr0, a
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

        org     %10
a:      ds      2
b:      ds      2

        org     %1000

arith:
        ld      r4, #a
        ld      r5, #b

        ldw     a, #18000
        ldw     b, #28000
        ld      r0, #'+'
        call    expr
        call    addsi2
        call    answer

        ldw     a, #18000
        ldw     b, #-18000
        ld      r0, #'+'
        call    expr
        call    addsi2
        call    answer

        ldw     a, #-28000
        ld      r0, #'+'
        call    expr
        call    addsi2
        call    answer

        ldw     a, #18000
        ldw     b, #-28000
        ld      r0, #'-'
        call    expr
        call    subsi2
        call    answer

        ldw     a, #18000
        ldw     b, #18000
        ld      r0, #'-'
        call    expr
        call    subsi2
        call    answer

        ldw     a, #-28000
        ld      r0, #'-'
        call    expr
        call    subsi2
        call    answer

        ldw     a, #300
        ldw     b, #-200
        ld      r0, #'*'
        call    expr
        call    mulsi2
        call    answer

        ldw     a, #100
        ldw     b, #-300
        ld      r0, #'*'
        call    expr
        call    mulsi2
        call    answer

        ldw     a, #-200
        ldw     b, #100
        ld      r0, #'*'
        call    expr
        call    mulsi2
        call    answer

        ldw     b, #100
        ld      r0, #'/'
        call    expr
        call    divsi2
        call    answer

        ldw     a, #30000
        ldw     b, #0
        ld      r0, #'/'
        call    expr
        call    divsi2
        call    answer

        ldw     a, #-30000
        ldw     b, #78
        ld      r0, #'/'
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
