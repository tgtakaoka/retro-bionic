*** -*- mode: asm; mode: flyspell-prog; -*-
        include "mn1613.inc"
*** i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     X'80'
USARTD: equ     USART+0         Data register
USARTS: equ     USART+1         Status register
USARTC: equ     USART+1         Control register
        include "../mn1610/i8251.inc"
* Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
* RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

        loc     RESET_PSW
        dc      X'0000'         STR: Disable interrupt
        dc      initialize      IC

        loc     X'0200'
stack:  equ     *-1
initialize:
        mvwi    SP, stack
        bal     init_usart
        bald    arith
        h                       halt to system

init_usart:
        * Initialize USART
        eor     R0, R0          R0=0
        wt      R0, USARTC
        wt      R0, USARTC
        wt      R0, USARTC      Safest way to sync mode
        mvi     R0, CMD_IR_bm
        wt      R0, USARTC      Reset
        mv      R0, R0          delay
        mv      R0, R0          delay
        mvi     R0, ASYNC_MODE
        wt      R0, USARTC
        mv      R0, R0          delay
        mv      R0, R0          delay
        mvi     R0, RX_EN_TX_EN
        wt      R0, USARTC      Enable Rx/Tx
        ret

newline:
        mvi     R0, X'0D'       CR
        bal     putchar
        mvi     R0, X'0A'       NL
putchar:
        push    R0
putchar_wait:
        rd      R0, USARTS
        tbit    R0, ST_TxRDY_bp, nz     Skip if TxRDY=1
        b       putchar_wait
        pop     R0
        wt      R0, USARTD
        ret

        include "arith.inc"

*** Print "VA op VB"
*** @param R0 op letter
*** @param R1 VA
*** @param R2 VB
*** @clobber R0 R1 R2
expr:
        push    R1
        push    R2
        push    R0
        mv      R0, R1
        bald    print_int16     print VA
        mvi     R0, ' '
        bald    putchar         print ' '
        pop     R0
        bald    putchar         print op
        mvi     R0, ' '
        bald    putchar         print ' '
        pop     R0
        push    R0              R0=VB
        bald    print_int16     print VB
        pop     R2              restore VB
        pop     R1              restore VA
        ret

*** Print " = value\n"
*** @param R1 value
*** @clobber R0 R1 R2
answer:
        mvi     R0, ' '
        bald    putchar         print ' '
        mvi     R0, '='
        bald    putchar         print '='
        mvi     R0, ' '
        bald    putchar         print ' '
        mv      R0, R1
        bald    print_int16     print value
        bd      newline

*** Print "VA rel VB\n"
*** @param R1 VA
*** @param R2 VB
*** @clobber R0 R1 R2
comp:
        c       R1, R2, mz      skip if VA<=VB
        b       comp_gt
        c       R1, R2, z       skip if VA==VB
        b       comp_lt
comp_eq:
        mvi     R0, '='
        b       comp_out
comp_gt:
        mvi     R0, '>'
        b       comp_out
comp_lt:
        mvi     R0, '<'
comp_out:
        bal     expr            print "VA rel VB"
        bd      newline

arith:
        mvwi    R1, 18000
        mvwi    R2, 28000
        mvi     R0, '+'
        bal     expr
        a       R1, R2          R1+=R2
        bal     answer          ; -19536

        mvwi    R1, 18000
        mvwi    R2, -18000
        mvi     R0, '+'
        bal     expr
        a       R1, R2          R1+=R2
        bal     answer          ; 0

        mvwi    R1, -18000
        mvwi    R2, -18000
        mvi     R0, '+'
        bal     expr
        a       R1, R2          R1+=R2
        bal     answer          ; 29536

        mvwi    R1, -18000
        mvwi    R2, -28000
        mvi     R0, '-'
        bal     expr
        s       R1, R2          R1-=R2
        bal     answer          ; 10000

        mvwi    R1, 18000
        mvwi    R2, -18000
        mvi     R0, '-'
        bal     expr
        s       R1, R2          R1-=R2
        bal     answer          ; -29536

        mvwi    R1, -28000
        mvwi    R1, -18000
        mvi     R0, '-'
        bal     expr
        s       R1, R2          R1-=R2
        bal     answer          ; -10000

        mvwi    R1, 100
        mvwi    R2, 300
        mvi     R0, '*'
        bal     expr
        bald    mul16
        bal     answer          ; 30000

        mvwi    R1, 200
        mvwi    R2, 100
        mvi     R0, '*'
        bal     expr
        bald    mul16
        bal     answer          ; 20000

        mvwi    R1, 300
        mvwi    R2, -200
        mvi     R0, '*'
        bal     expr
        bald    mul16
        bal     answer          ; 5536

        mvwi    R1, 100
        mvwi    R2, -300
        mvi     R0, '*'
        bald    expr
        bald    mul16
        bal     answer          ; -30000

        mvwi    R1, -200
        mvwi    R2, -100
        mvi     R0, '*'
        bald    expr
        bald    mul16
        bal     answer          ; 20000

        mvwi    R1, 30000
        mvwi    R2, 100
        mvi     R0, '/'
        bald    expr
        bald    div16
        bald    answer          ; 300

        mvwi    R1, -200
        mvwi    R2, 100
        mvi     R0, '/'
        bald    expr
        bald    div16
        bald    answer          ; -2

        mvwi    R1, -30000
        mvwi    R2, -200
        mvi     R0, '/'
        bald    expr
        bald    div16
        bald    answer          ; 150

        mvwi    R1, -30000
        mvwi    R2, 78
        mvi     R0, '/'
        bald    expr
        bald    div16
        bald    answer          ; -384

        mvwi    R1, 5000
        mvwi    R2, 4000
        bald    comp

        mvwi    R1, 5000
        mvwi    R2, 5000
        bald    comp

        mvwi    R1, 4000
        mvwi    R2, 5000
        bald    comp

        mvwi    R1, -5000
        mvwi    R2, -4000
        bald    comp

        mvwi    R1, -5000
        mvwi    R2, -5000
        bald    comp

        mvwi    R1, -4000
        mvwi    R2, -5000
        bald    comp

        mvwi    R1, 32700
        mvwi    R2, 32600
        bald    comp

        mvwi    R1, 32700
        mvwi    R2, 32700
        bald    comp

        mvwi    R1, 32600
        mvwi    R2, 32700
        bald    comp

        mvwi    R1, -32700
        mvwi    R2, -32600
        bald    comp

        mvwi    R1, -32700
        mvwi    R2, -32700
        bald    comp

        mvwi    R1, -32600
        mvwi    R2, -32700
        bald    comp

        mvwi    R1, 18000
        mvwi    R2, -28000
        bald    comp

        mvwi    R1, -28000
        mvwi    R2, -28000
        bald    comp

        mvwi    R1, -28000
        mvwi    R2, 18000
        bald    comp

        ret
