;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tlcs90
        include "tmp90c802.inc"

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

        org     1000H
stack:  equ     $

        org     ORG_RESET
        ld      SP, stack
        jp      init_usart

        org     ORG_SWI
        halt                    ; halt to system

        org     0100H
init_usart:
        ld      (USARTC), 0
        ld      (USARTC), 0
        ld      (USARTC), 0     ; safest way to sync mode
        ld      (USARTC), CMD_IR_bm
        nop
        nop
        ld      (USARTC), ASYNC_MODE
        nop
        nop
        ld      (USARTC), RX_EN_TX_EN

        call    arith
        swi

putchar:
        bit     ST_TxRDY_bp, (USARTS)
        jr      Z, putchar
        ld      (USARTD), A
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
;;; @param v1: BC
;;; @param v2: DE
;;; @param op: A
;;; @return v1: HL
;;; @clobber A
expr:
        push    AF
        ld      HL, BC
        call    print_int16
        call    putspace
        pop     AF
        call    putchar
        call    putspace
        ld      HL, DE
        call    print_int16
        ld      HL, BC
        ret

;;; Print " v1\n"
;;; @param v1: HL
;;; @clobber A HL
answer:
        call    putspace
        ld      A, '='
        call    putchar
        call    putspace
        call    print_int16
        jr      newline

;;; Compare and print "v1 rel v2\n"
;;; @param v1: BC
;;; @param v2: DE
;;; @clobber A H:
comp:
        ld      HL, BC
        cp      HL, DE
        jr      Z, comp_eq
        jr      GT, comp_gt
        jr      LT, comp_lt
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
        call    expr
        jr      newline

        org     1000H
arith:
        ld      BC, 0
        ld      DE, -28000
        ld      A, '-'
        call    expr
        sub     HL, DE
        call    answer          ; 28000

        ld      BC, 0
        ld      DE, 28000
        ld      A, '-'
        call    expr
        sub     HL, DE
        call    answer          ; -28000

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
        sub     HL, DE
        call    answer          ; 10000

        ld      BC, 100
        ld      DE, 300
        ld      A, '*'
        call    expr
        call    mulsi2
        call    answer          ; 30000

        ld      BC, 300
        ld      DE, -200
        ld      A, '*'
        call    expr
        call    mulsi2
        call    answer          ; 5536

        ld      BC, 100
        ld      DE, -300
        ld      A, '*'
        call    expr
        call    mulsi2
        call    answer          ; -30000

        ld      BC, -200
        ld      DE, -100
        ld      A, '*'
        call    expr
        call    mulsi2
        call    answer          ; 20000

        ld      BC, 30000
        ld      DE, 100
        ld      A, '/'
        call    expr
        call    divsi2
        call    answer          ; 300

        ld      BC, -200
        ld      DE, 100
        ld      A, '/'
        call    expr
        call    divsi2
        call    answer          ; -2

        ld      BC, -30000
        ld      DE, -200
        ld      A, '/'
        call    expr
        call    divsi2
        call    answer          ; 150

        ld      BC, -30000
        ld      DE, 78
        ld      A, '/'
        call    expr
        call    divsi2
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
