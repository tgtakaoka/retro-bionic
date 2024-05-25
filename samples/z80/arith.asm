;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z80
        include "z80.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     00H
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

expr:
        push    AF
        ld      A, (BC)
        ld      L, A
        inc     BC
        ld      A, (BC)
        ld      H, A
        dec     BC              ; HL=@BC
        call    print_int16
        call    putspace
        pop     AF
        call    putchar
        call    putspace
        ld      A, (DE)
        ld      L, A
        inc     DE
        ld      A, (DE)
        ld      H, A
        dec     DE
        jp      print_int16     ; HL=@DE

answer:
        call    putspace
        ld      A, '='
        call    putchar
        call    putspace
        ld      HL, (vA)
        call    print_int16
        jr      newline

comp:
        ld      BC, vA
        ld      DE, vB
        call    cmpsi2
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
        call    expr
        jr      newline

        org     0200H

vA:     ds      2
vB:     ds      2

        org     1000H

arith:
        ld      BC, vA
        ld      DE, vB

        ld      HL, 0
        ld      (vA), HL
        ld      HL, -28000
        ld      (vB), HL
        ld      A, '-'
        call    expr
        call    negsi2
        call    answer          ; 28000

        ld      HL, 0
        ld      (vA), HL
        ld      HL, 28000
        ld      (vB), HL
        ld      A, '-'
        call    expr
        call    negsi2
        call    answer          ; -28000

        ld      HL, 18000
        ld      (vA), HL
        ld      HL, 28000
        ld      (vB), HL
        ld      A, '+'
        call    expr
        call    addsi2
        call    answer          ; -19536

        ld      HL, 18000
        ld      (vA), HL
        ld      HL, -18000
        ld      (vB), HL
        ld      A, '+'
        call    expr
        call    addsi2
        call    answer          ; 0

        ld      HL, -18000
        ld      (vA), HL
        ld      HL, -18000
        ld      (vB), HL
        ld      A, '+'
        call    expr
        call    addsi2
        call    answer          ; 29536

        ld      HL, -18000
        ld      (vA), HL
        ld      HL, -28000
        ld      (vB), HL
        ld      A, '-'
        call    expr
        call    subsi2
        call    answer          ; -10000

        ld      HL, 100
        ld      (vA), HL
        ld      HL, 300
        ld      (vB), HL
        ld      A, '*'
        call    expr
        call    mulsi2
        call    answer          ; 30000

        ld      HL, 300
        ld      (vA), HL
        ld      HL, -200
        ld      (vB), HL
        ld      A, '*'
        call    expr
        call    mulsi2
        call    answer          ; 5536

        ld      HL, 100
        ld      (vA), HL
        ld      HL, -300
        ld      (vB), HL
        ld      A, '*'
        call    expr
        call    mulsi2
        call    answer          ; -30000

        ld      HL, -200
        ld      (vA), HL
        ld      HL, -100
        ld      (vB), HL
        ld      A, '*'
        call    expr
        call    mulsi2
        call    answer          ; 20000

        ld      HL, 30000
        ld      (vA), HL
        ld      HL, 100
        ld      (vB), HL
        ld      A, '/'
        call    expr
        call    divsi2
        call    answer          ; 30

        ld      HL, -200
        ld      (vA), HL
        ld      HL, 100
        ld      (vB), HL
        ld      A, '/'
        call    expr
        call    divsi2
        call    answer          ; -2

        ld      HL, -30000
        ld      (vA), HL
        ld      HL, -200
        ld      (vB), HL
        ld      A, '/'
        call    expr
        call    divsi2
        call    answer          ; 150

        ld      HL, -30000
        ld      (vA), HL
        ld      HL, 78
        ld      (vB), HL
        ld      A, '/'
        call    expr
        call    divsi2
        call    answer          ; -384

        ld      HL, -48
        ld      (vA), HL
        ld      HL, 30
        ld      (vB), HL
        call    comp

        ld      HL, 30
        ld      (vA), HL
        ld      HL, -48
        ld      (vB), HL
        call    comp

        ld      HL, 5000
        ld      (vA), HL
        ld      HL, 4000
        ld      (vB), HL
        call    comp

        ld      HL, 5000
        ld      (vB), HL
        call    comp

        ld      HL, 4000
        ld      (vA), HL
        call    comp

        ld      HL, -5000
        ld      (vA), HL
        ld      HL, -4000
        ld      (vB), HL
        call    comp

        ld      HL, -5000
        ld      (vB), HL
        call    comp

        ld      HL, -4000
        ld      (vA), HL
        call    comp

        ld      HL, 32700
        ld      (vA), HL
        ld      HL, 32600
        ld      (vB), HL
        call    comp

        ld      HL, 32700
        ld      (vB), HL
        call    comp

        ld      HL, 32600
        ld      (vA), HL
        call    comp

        ld      HL, -32700
        ld      (vA), HL
        ld      HL, -32600
        ld      (vB), HL
        call    comp

        ld      HL, -32700
        ld      (vB), HL
        call    comp

        ld      HL, -32600
        ld      (vA), HL
        call    comp

        ld      HL, 18000
        ld      (vA), HL
        ld      HL, -28000
        ld      (vB), HL
        call    comp

        ld      HL, 18000
        ld      (vB), HL
        call    comp

        ld      HL, -28000
        ld      (vA), HL
        call    comp

        ret

        include "arith.inc"

        end
