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

        loc     ZERO_PAGE
FR0:    equ     *               Float Register
FR0H:   ds      1
FR0L:   ds      1

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

*** Print "VA op VB" and convert to float
*** @param R0 op letter
*** @param R1 VA
*** @param R2 VB
*** @return DR0 float(VA)
*** @return FR0 float(VB)
*** @return R2 address of FR0
expr:
        push    R1              save VA
        push    R2              save VB
        push    R0              save op letter
        mv      R0, R1
        bald    print_int16     print VA
        mvi     R0, ' '
        bald    putchar         print ' '
        pop     R0              restore op letter
        bald    putchar         print op
        mvi     R0, ' '
        bald    putchar         print ' '
        pop     R2              restore VB
        mv      R0, R2
        flt     DR0, R0         DR0=float(VB)
        st      R0, FR0H
        st      R1, FR0L
        mv      R0, R2
        bald    print_int16     print VB
        pop     R0              restore VA
        flt     DR0, R0         DR0=float(VA)
        mvwi    R2, FR0
        ret

*** Print " = value\n"
*** @param DR0 float value
*** @clobber DR0 R2
answer:
        push    R0              save value
        push    R1
        mvi     R0, ' '
        bald    putchar         print ' '
        mvi     R0, '='
        bald    putchar         print '='
        mvi     R0, ' '
        bald    putchar         print ' '
        pop     R1
        pop     R0              restore value
        fix     R0, DR0         R0=int(DR0)
        bald    print_int16     print value
        bd      newline

*** Print "VA rel VB\n" with float comparison
*** @param R1 VA
*** @param R2 VB
*** @clobber R0 R1 R2 FR0
comp:
        push    R2              save VB
        push    R1              save VA
        mv      R0, R2
        flt     DR0, R0         DR0=float(VB)
        st      R0, FR0H
        st      R1, FR0L        FR0=VB
        pop     R0
        push    R0              R0=VA
        flt     DR0, R0         DR0=float(VA)
        mvwi    R2, FR0
        fs      DR0, (R2)       VA-VB
        mv      R0,R0, mz       skip if DR0<=0
        b       comp_gt
        mv      R0,R0, z        skip if DR0=0
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
        pop     R1              restore VA
        pop     R2              restore VB
        bal     expr            print "VA rel VB"
        bd      newline

arith:
        mvwi    R1, 18000
        mvwi    R2, 28000
        mvi     R0, '+'
        bal     expr
        fa      DR0, (R2)       DR0+=FR0
        bal     answer          ; -19536

        mvwi    R1, 18000
        mvwi    R2, -18000
        mvi     R0, '+'
        bal     expr
        fa      DR0, (R2)       DR0+=FR0
        bal     answer          ; 0

        mvwi    R1, -18000
        mvwi    R2, -18000
        mvi     R0, '+'
        bal     expr
        fa      DR0, (R2)       DR0+=FR0
        bal     answer          ; 29536

        mvwi    R1, -18000
        mvwi    R2, -28000
        mvi     R0, '-'
        bal     expr
        fs      DR0, (R2)       DR0-=FR0
        bal     answer          ; 10000

        mvwi    R1, 18000
        mvwi    R2, -18000
        mvi     R0, '-'
        bal     expr
        fs      DR0, (R2)       DR0-=FR0
        bal     answer          ; -29536

        mvwi    R1, -28000
        mvwi    R1, -18000
        mvi     R0, '-'
        bal     expr
        fs      DR0, (R2)       DR0-=FR0
        bal     answer          ; -10000

        mvwi    R1, 100
        mvwi    R2, 300
        mvi     R0, '*'
        bal     expr
        fm      DR0, (R2)       DR0*=FR0
        bal     answer          ; 30000

        mvwi    R1, 200
        mvwi    R2, 100
        mvi     R0, '*'
        bald    expr
        fm      DR0, (R2)       DR0*=FR0
        bal     answer          ; 20000

        mvwi    R1, 300
        mvwi    R2, -200
        mvi     R0, '*'
        bald    expr
        fm      DR0, (R2)       DR0*=FR0
        bal     answer          ; 5536

        mvwi    R1, 100
        mvwi    R2, -300
        mvi     R0, '*'
        bald    expr
        fm      DR0, (R2)       DR0*=FR0
        bal     answer          ; -30000

        mvwi    R1, -200
        mvwi    R2, -100
        mvi     R0, '*'
        bald    expr
        fm      DR0, (R2)       DR0*=FR0
        bald    answer          ; 20000

        mvwi    R1, 30000
        mvwi    R2, 100
        mvi     R0, '/'
        bald    expr
        fd      DR0, (R2)       DR0/=FR0
        bald    answer          ; 300

        mvwi    R1, -200
        mvwi    R2, 100
        mvi     R0, '/'
        bald    expr
        fd      DR0, (R2)       DR0/=FR0
        bald    answer          ; -2

        mvwi    R1, -30000
        mvwi    R2, -200
        mvi     R0, '/'
        bald    expr
        fd      DR0, (R2)       DR0/=FR0
        bald    answer          ; 150

        mvwi    R1, -30000
        mvwi    R2, 78
        mvi     R0, '/'
        bald    expr
        fd      DR0, (R2)       DR0/=FR0
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
