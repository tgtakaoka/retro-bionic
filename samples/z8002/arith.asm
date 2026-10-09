;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z8002.inc"
        include "usart.inc"

stack:  equ     1000H

        org     ORG_RESET
        word    0
        word    FCW_SN          ; system mode, interrupts disabled
        word    init_usart

        org     0100H
init_usart:
        ld      r15, #stack
        clrb    rl0
        outb    USARTC, rl0
        outb    USARTC, rl0
        outb    USARTC, rl0     ; safest way to sync mode
        ldb     rl0, #CMD_IR_bm
        outb    USARTC, rl0     ; reset
        nop
        nop
        ldb     rl0, #ASYNC_MODE
        outb    USARTC, rl0
        nop
        nop
        ldb     rl0, #RX_EN_TX_EN
        outb    USARTC, rl0

        call    arith
halt_to_system:
        sc      #SC_EXIT

;;; Put character
;;; @param RL0
putchar:
        push    @r15, r0
putchar_loop:
        inb     rl0, USARTS
        bitb    rl0, #ST_TxRDY_bp
        jr      z, putchar_loop
        pop     r0, @r15
        outb    USARTD, rl0
        ret

newline:
        push    @r15, r0
        ldb     rl0, #0DH
        call    putchar
        ldb     rl0, #0AH
        call    putchar
        pop     r0, @r15
        ret

putspace:
        push    @r15, r0
        ldb     rl0, #' '
        call    putchar
        pop     r0, @r15
        ret

;;; Print "v1 op v2"
;;; @param R4 v1
;;; @param R5 v2
;;; @param RL0 op
;;; @return R1 v1
;;; @clobber R0 R2 R3
expr:
        push    @r15, r0
        ld      r1, r4
        call    print_int16
        call    putspace
        pop     r0, @r15
        call    putchar
        call    putspace
        ld      r1, r5
        call    print_int16
        ld      r1, r4
        ret

;;; Print " = v1\n"
;;; @param R1 v1
;;; @clobber R0 R1 R2 R3
answer:
        call    putspace
        ldb     rl0, #'='
        call    putchar
        call    putspace
        call    print_int16
        jr      newline

;;; Compare and print "v1 rel v2\n"
;;; @param R4 v1
;;; @param R5 v2
;;; @clobber R0 R1 R2 R3
comp:
        cp      r4, r5
        jr      eq, comp_eq
        jr      gt, comp_gt
        jr      lt, comp_lt
        ldb     rl0, #'?'
        jr      comp_out
comp_gt:
        ldb     rl0, #'>'
        jr      comp_out
comp_eq:
        ldb     rl0, #'='
        jr      comp_out
comp_lt:
        ldb     rl0, #'<'
comp_out:
        call    expr
        jr      newline

        include "arith.inc"

        org     1000H
arith:
        ld      r4, #18000
        ld      r5, #28000
        ldb     rl0, #'+'
        call    expr
        add     r1, r5
        call    answer          ; -19536

        ld      r4, #18000
        ld      r5, #-18000
        ldb     rl0, #'+'
        call    expr
        add     r1, r5
        call    answer          ; 0

        ld      r4, #-18000
        ld      r5, #-18000
        ldb     rl0, #'+'
        call    expr
        add     r1, r5
        call    answer          ; 29536

        ld      r4, #-18000
        ld      r5, #-28000
        ldb     rl0, #'-'
        call    expr
        sub     r1, r5
        call    answer          ; 10000

        ld      r4, #100
        ld      r5, #300
        ldb     rl0, #'*'
        call    expr
        call    mul16
        call    answer          ; 30000

        ld      r4, #300
        ld      r5, #-200
        ldb     rl0, #'*'
        call    expr
        call    mul16
        call    answer          ; 5536

        ld      r4, #100
        ld      r5, #-300
        ldb     rl0, #'*'
        call    expr
        call    mul16
        call    answer          ; -30000

        ld      r4, #-200
        ld      r5, #-100
        ldb     rl0, #'*'
        call    expr
        call    mul16
        call    answer          ; 20000

        ld      r4, #30000
        ld      r5, #100
        ldb     rl0, #'/'
        call    expr
        call    div16
        call    answer          ; 300

        ld      r4, #-200
        ld      r5, #100
        ldb     rl0, #'/'
        call    expr
        call    div16
        call    answer          ; -2

        ld      r4, #-30000
        ld      r5, #-200
        ldb     rl0, #'/'
        call    expr
        call    div16
        call    answer          ; 150

        ld      r4, #-30000
        ld      r5, #78
        ldb     rl0, #'/'
        call    expr
        call    div16
        call    answer          ; -384

        ld      r4, #-48
        ld      r5, #30
        call    comp

        ld      r4, #30
        ld      r5, #-48
        call    comp

        ld      r4, #5000
        ld      r5, #4000
        call    comp

        ld      r4, #5000
        ld      r5, #5000
        call    comp

        ld      r4, #4000
        ld      r5, #5000
        call    comp

        ld      r4, #-5000
        ld      r5, #-4000
        call    comp

        ld      r4, #-5000
        ld      r5, #-5000
        call    comp

        ld      r4, #-4000
        ld      r5, #-5000
        call    comp

        ld      r4, #32700
        ld      r5, #32600
        call    comp

        ld      r4, #32700
        ld      r5, #32700
        call    comp

        ld      r4, #32600
        ld      r5, #32700
        call    comp

        ld      r4, #-32700
        ld      r5, #-32600
        call    comp

        ld      r4, #-32700
        ld      r5, #-32700
        call    comp

        ld      r4, #-32600
        ld      r5, #-32700
        call    comp

        ld      r4, #18000
        ld      r5, #-28000
        call    comp

        ld      r4, #18000
        ld      r5, #18000
        call    comp

        ld      r4, #-28000
        ld      r5, #18000
        call    comp

        ret

;;; Signed multiplication, the low 16 bits
;;; @param R1 multiplicand
;;; @param R5 multiplier
;;; @return R1 product
;;; @clobber R2 R3
mul16:
        ld      r3, r1
        mult    rr2, r5         ; RR2=R3*R5
        ld      r1, r3
        ret

;;; Signed division, rounded toward zero
;;; @param R1 dividend
;;; @param R5 divisor
;;; @return R1 quotient
;;; @clobber R2 R3
div16:
        ld      r3, r1
        exts    rr2             ; RR2=dividend, sign-extended
        div     rr2, r5         ; R3=quotient, R2=remainder
        ld      r1, r3
        ret
