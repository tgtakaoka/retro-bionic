;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "i8096.inc"
        include "usart.inc"

        org     1000H
stack:  equ     $


        org     1AH
A:      dsb     1
B:      dsb     1
HL:     dsw     1
QHL:    dsw     1               ; 32-bit for mul/div
DE:     dsw     1

        org     ORG_RESET
init:
        ld      SP, #stack
        ldb     INT_MASK, INT_EXTINT ; enable EXTINT
        ei                           ; for halt switch

init_usart:
        stb     ZERO, USARTC
        stb     ZERO, USARTC
        stb     ZERO, USARTC    ; safest way to sync mode
        ldb     A, #CMD_IR_bm
        stb     A, USARTC       ; reset
        nop
        nop
        ldb     A, #ASYNC_MODE
        stb     A, USARTC
        nop
        nop
        ldb     A, #RX_EN_TX_EN
        stb     A, USARTC

        scall   arith

halt_to_system:
        ld      A, #VEC_TRAP
        st      A, VEC_TRAP     ; halt to system
        trap

putchar:
        push    A
putchar_loop:
        ldb     A, USARTS
        jbc     A, ST_TxRDY_bp, putchar_loop
        pop     A
        stb     A, USARTD
        ret

newline:
        push    A
        ldb     A, #0DH
        scall   putchar
        ldb     A, #0AH
        scall   putchar
        pop     A
        ret

putspace:
        push    A
        ldb     A, #' '
        scall   putchar
        pop     A
        ret

;;; Print "v1 op v2 = "
;;; @param v1 HL
;;; @param v2 DE
;;; @param op A
;;; @clobber A
expr:
        push    HL
        push    A
        scall   print_int16
        scall   putspace
        ld      A, [SP]         ; restore A
        scall   putchar
        scall   putspace
        ld      HL, DE
        scall   print_int16
        pop     A
        pop     HL
        ret

;;; Print " v1\n"
;;; @param v1 HL
;;; @clobber A HL
answer:
        scall   putspace
        ldb     A, #'='
        scall   putchar
        scall   putspace
        scall   print_int16
        sjmp    newline

;;; Compare and print "v1 rel v2\n"
;;; @param v1 HL
;;; @param v2 DE
;;; @clobber A
comp:
        cmp     HL, DE
        je      comp_eq
        jgt     comp_gt
        jlt     comp_lt
        ldb     A, #'?'
        sjmp    comp_out
comp_gt:
        ldb     A, #'>'
        sjmp    comp_out
comp_eq:
        ldb     A, #'='
        sjmp    comp_out
comp_lt:
        ldb     A, #'<'
comp_out:
        scall   expr
        sjmp    newline

arith:
        ld      HL, #18000
        ld      DE, #28000
        ldb     A, #'+'
        scall   expr
        add     HL, DE
        scall   answer          ; -19536

        ld      HL, #18000
        ld      DE, #-18000
        ldb     A, #'+'
        scall   expr
        add     HL, DE
        scall   answer          ; 0

        ld      HL, #-18000
        ld      DE, #-18000
        ldb     A, #'+'
        scall   expr
        add     HL, DE
        scall   answer          ; 29536

        ld      HL, #-18000
        ld      DE, #-28000
        ldb     A, #'-'
        scall    expr
        sub     HL, DE
        scall   answer          ; -10000

        ld      HL, #100
        ld      DE, #300
        ldb     A, #'*'
        scall   expr
        mul     HL, DE
        scall   answer          ; 30000

        ld      HL, #300
        ld      DE, #-200
        ldb     A, #'*'
        scall   expr
        mul     HL, DE
        scall   answer          ; 5536

        ld      HL, #100
        ld      DE, #-300
        ldb     A, #'*'
        scall   expr
        mul     HL, DE
        scall   answer          ; -30000

        ld      HL, #-200
        ld      DE, #-100
        ldb     A, #'*'
        scall   expr
        mul     HL, DE
        scall   answer          ; 20000

        ld      HL, #30000
        ld      DE, #100
        ldb     A, #'/'
        scall   expr
        ext     HL
        div     HL, DE
        scall   answer          ; 30

        ld      HL, #-200
        ld      DE, #100
        ldb     A, #'/'
        scall   expr
        ext     HL
        div     HL, DE
        scall   answer          ; -2

        ld      HL, #-30000
        ld      DE, #-200
        ldb     A, #'/'
        scall   expr
        ext     HL
        div     HL, DE
        scall   answer          ; 150

        ld      HL, #-30000
        ld      DE, #78
        ldb     A, #'/'
        scall   expr
        ext     HL
        div     HL, DE
        scall   answer          ; -384

        ld      HL, #-48
        ld      DE, #30
        scall   comp

        ld      HL, #30
        ld      DE, #-48
        scall   comp

        ld      HL, #5000
        ld      DE, #4000
        scall   comp

        ld      HL, #5000
        ld      DE, #5000
        scall   comp

        ld      HL, #4000
        ld      DE, #5000
        scall   comp

        ld      HL, #-5000
        ld      DE, #-4000
        scall   comp

        ld      HL, #-5000
        ld      DE, #-5000
        scall   comp

        ld      HL, #-4000
        ld      DE, #-5000
        scall   comp

        ld      HL, #32700
        ld      DE, #32600
        scall   comp

        ld      HL, #32700
        ld      DE, #32700
        scall   comp

        ld      HL, #32600
        ld      DE, #32700
        scall   comp

        ld      HL, #-32700
        ld      DE, #-32600
        scall   comp

        ld      HL, #-32700
        ld      DE, #-32700
        scall   comp

        ld      HL, #-32600
        ld      DE, #-32700
        scall   comp

        ld      HL, #18000
        ld      DE, #-28000
        scall   comp

        ld      HL, #18000
        ld      DE, #18000
        scall   comp

        ld      HL, #-28000
        ld      DE, #18000
        scall   comp

        ret

        include "arith.inc"

        end
