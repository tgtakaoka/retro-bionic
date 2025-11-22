;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z180.inc"
        include "../z80/usart.inc"

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jp      init_usart

        org     0100H
init_usart:
        ld      sp, stack
        xor     A               ; clear A
        out     (USARTC), A
        out     (USARTC), A
        out     (USARTC), A          ; safest way to sync mode
        ld      A, CMD_IR_bm
        out     (USARTC), A          ; reset
        nop
        nop
        ld      A, ASYNC_MODE
        out     (USARTC), A
        nop
        nop
        ld      A, RX_EN_TX_EN
        out     (USARTC), A

        call    arith
        halt

putchar:
        push    AF
putchar_loop:
        in      A, (USARTS)
        bit     ST_TxRDY_bp, A
        jr      Z, putchar_loop
        pop     AF
        out     (USARTD), A
        ret

newline:
        push    AF
        ld      A, 0DH
        call    putchar
        ld      A, 0AH
        call    putchar
        pop     AF
        ret

putspace:
        push    AF
        ld      A, ' '
        call    putchar
        pop     AF
        ret

;;; Print "v1 op v2 = "
;;; @param v1 BC
;;; @param v2 DE
;;; @param op A
;;; @return v1 HL
;;; @clobber A
expr:
        push    AF
        ld      H, B
        ld      L, C
        call    print_int16
        call    putspace
        pop     AF
        call    putchar
        call    putspace
        ld      H, D
        ld      L, E
        call    print_int16
        ld      H, B
        ld      L, C
        ret

;;; Print " v1\n"
;;; @param v1 HL
;;; @clobber A HL
answer:
        call    putspace
        ld      A, '='
        call    putchar
        call    putspace
        call    print_int16
        jr      newline

;;; Compare and print "v1 rel v2\n"
;;; @param v1 BC
;;; @param v2 DE
;;; @clobber A HL
comp:
        push    BC
        push    DE
        call    cmp16
        jr      Z, comp_eq
        jp      P, comp_gt
        jp      M, comp_lt
        ld      A, '?'
        jr      comp_out
comp_gt:
        ld      A, '>'
        jr      comp_out
comp_eq:
        ld      A, '='
        jr      comp_out
comp_lt:
        ld      A, '<'
comp_out:
        pop     DE
        pop     BC
        call    expr
        jr      newline

        org     1000H

arith:
        ld      BC, 18000
        ld      DE, 28000
        ld      A, '+'
        call    expr
        add     HL, DE
        call    answer          ; -19536

        ld      BC, 18000
        ld      DE, -18000
        ld      A, '+'
        call    expr
        add     HL, DE
        call    answer          ; 0

        ld      BC, -18000
        ld      DE, -18000
        ld      A, '+'
        call    expr
        add     HL, DE
        call    answer          ; 29536

        ld      BC, -18000
        ld      DE, -28000
        ld      A, '-'
        call    expr
        scf
        ccf
        sbc     HL, DE
        call    answer          ; -10000

        ld      BC, 100
        ld      DE, 300
        ld      A, '*'
        call    expr
        call    mul16
        call    answer          ; 30000

        ld      BC, 300
        ld      DE, -200
        ld      A, '*'
        call    expr
        call    mul16
        call    answer          ; 5536

        ld      BC, 100
        ld      DE, -300
        ld      A, '*'
        call    expr
        call    mul16
        call    answer          ; -30000

        ld      BC, -200
        ld      DE, -100
        ld      A, '*'
        call    expr
        call    mul16
        call    answer          ; 20000

        ld      BC, 30000
        ld      DE, 100
        ld      A, '/'
        call    expr
        call    div16
        call    answer          ; 30

        ld      BC, -200
        ld      DE, 100
        ld      A, '/'
        call    expr
        call    div16
        call    answer          ; -2

        ld      BC, -30000
        ld      DE, -200
        ld      A, '/'
        call    expr
        call    div16
        call    answer          ; 150

        ld      BC, -30000
        ld      DE, 78
        ld      A, '/'
        call    expr
        call    div16
        call    answer          ; -384

        ld      BC, -48
        ld      DE, 30
        call    comp

        ld      BC, 30
        ld      DE, -48
        call    comp

        ld      BC, 5000
        ld      DE, 4000
        call    comp

        ld      BC, 5000
        ld      DE, 5000
        call    comp

        ld      BC, 4000
        ld      DE, 5000
        call    comp

        ld      BC, -5000
        ld      DE, -4000
        call    comp

        ld      BC, -5000
        ld      DE, -5000
        call    comp

        ld      BC, -4000
        ld      DE, -5000
        call    comp

        ld      BC, 32700
        ld      DE, 32600
        call    comp

        ld      BC, 32700
        ld      DE, 32700
        call    comp

        ld      BC, 32600
        ld      DE, 32700
        call    comp

        ld      BC, -32700
        ld      DE, -32600
        call    comp

        ld      BC, -32700
        ld      DE, -32700
        call    comp

        ld      BC, -32600
        ld      DE, -32700
        call    comp

        ld      BC, 18000
        ld      DE, -28000
        call    comp

        ld      BC, 18000
        ld      DE, 18000
        call    comp

        ld      BC, -28000
        ld      DE, 18000
        call    comp

        ret

        include "arith.inc"

        end
