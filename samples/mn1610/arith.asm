*** -*- mode: asm; mode: flyspell-prog; -*-
        include "mn1610.inc"
*** i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:  equ     X'80'
USARTD: equ     USART+0         Data register
USARTS: equ     USART+1         Status register
USARTC: equ     USART+1         Control register
        include "i8251.inc"
* Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
* RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm

        loc     RESET_PSW
        dc      X'0000'         STR: Disable interrupt
        dc      initialize      IC

        loc     ZERO_PAGE
I_putchar:      dc      putchar
I_newline:      dc      newline
I_print_int16:  dc      print_int16
I_mul16:        dc      mul16
I_div16:        dc      div16
I_arith:        dc      arith
I_expr:         dc      expr
I_answer:       dc      answer
I_comp:         dc      comp

        loc     X'0200'
stack:  equ     *-1
initialize:
        mvi     SP, hi(stack)
        bswp    SP, SP
        mvi     SP, lo(stack)
        bal     init_usart
        bal     (I_arith)
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
*** @clobber R0
expr:
        push    R1
        push    R2
        push    R0
        mv      R0, R1
        bal     (I_print_int16) print VA
        mvi     R0, ' '
        bal     (I_putchar)     print ' '
        pop     R0
        bal     (I_putchar)     print op
        mvi     R0, ' '
        bal     (I_putchar)     print ' '
        pop     R2
        push    R2
        mv      R0, R2
        bal     (I_print_int16) print VB
        pop     R2
        pop     R1
        ret

*** Print " = value\n"
*** @param R1 value
*** @clobber R1
answer:
        mvi     R0, ' '
        bal     (I_putchar)     print ' '
        mvi     R0, '='
        bal     (I_putchar)     print '='
        mvi     R0, ' '
        bal     (I_putchar)     print ' '
        mv      R0, R1
        bal     (I_print_int16) print value
        b       (I_newline)

*** Print "VA rel VB\n"
*** @param R1 VA
*** @param R2 VB
*** @clobber R0
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
        b       (I_newline)

arith:
        mvi     R1, hi(18000)
        bswp    R1, R1
        mvi     R1, lo(18000)
        mvi     R2, hi(28000)
        bswp    R2, R2
        mvi     R2, lo(28000)
        mvi     R0, '+'
        bal     expr
        a       R1, R2          R1+=R2
        bal     answer          ; -19536

        mvi     R1, hi(18000)
        bswp    R1, R1
        mvi     R1, lo(18000)
        mvi     R2, hi(-18000)
        bswp    R2, R2
        mvi     R2, lo(-18000)
        mvi     R0, '+'
        bal     expr
        a       R1, R2          R1+=R2
        bal     answer          ; 0

        mvi     R1, hi(-18000)
        bswp    R1, R1
        mvi     R1, lo(-18000)
        mvi     R2, hi(-18000)
        bswp    R2, R2
        mvi     R2, lo(-18000)
        mvi     R0, '+'
        bal     expr
        a       R1, R2          R1+=R2
        bal     answer          ; 29536

        mvi     R1, hi(-18000)
        bswp    R1, R1
        mvi     R1, lo(-18000)
        mvi     R2, hi(-28000)
        bswp    R2, R2
        mvi     R2, lo(-28000)
        mvi     R0, '-'
        bal     expr
        s       R1, R2          R1-=R2
        bal     answer          ; 10000

        mvi     R1, hi(18000)
        bswp    R1, R1
        mvi     R1, lo(18000)
        mvi     R2, hi(-18000)
        bswp    R2, R2
        mvi     R2, lo(-18000)
        mvi     R0, '-'
        bal     expr
        s       R1, R2          R1-=R2
        bal     answer          ; -29536

        mvi     R1, hi(-28000)
        bswp    R1, R1
        mvi     R1, lo(-28000)
        mvi     R2, hi(-18000)
        bswp    R2, R2
        mvi     R2, lo(-18000)
        mvi     R0, '-'
        bal     expr
        s       R1, R2          R1-=R2
        bal     answer          ; -10000

        mvi     R1, hi(100)
        bswp    R1, R1
        mvi     R1, lo(100)
        mvi     R2, hi(300)
        bswp    R2, R2
        mvi     R2, lo(300)
        mvi     R0, '*'
        bal     expr
        bal     (I_mul16)
        bal     answer          ; 30000

        mvi     R1, hi(200)
        bswp    R1, R1
        mvi     R1, lo(200)
        mvi     R2, hi(100)
        bswp    R2, R2
        mvi     R2, lo(100)
        mvi     R0, '*'
        bal     expr
        bal     (I_mul16)
        bal     answer          ; 20000

        mvi     R1, hi(300)
        bswp    R1, R1
        mvi     R1, lo(300)
        mvi     R2, hi(-200)
        bswp    R2, R2
        mvi     R2, lo(-200)
        mvi     R0, '*'
        bal     expr
        bal     (I_mul16)
        bal     answer          ; 5536

        mvi     R1, hi(100)
        bswp    R1, R1
        mvi     R1, lo(100)
        mvi     R2, hi(-300)
        bswp    R2, R2
        mvi     R2, lo(-300)
        mvi     R0, '*'
        bal     (I_expr)
        bal     (I_mul16)
        bal     answer          ; -30000

        mvi     R1, hi(-200)
        bswp    R1, R1
        mvi     R1, lo(-200)
        mvi     R2, hi(-100)
        bswp    R2, R2
        mvi     R2, lo(-100)
        mvi     R0, '*'
        bal     (I_expr)
        bal     (I_mul16)
        bal     (I_answer)      ; 20000

        mvi     R1, hi(30000)
        bswp    R1, R1
        mvi     R1, lo(30000)
        mvi     R2, hi(100)
        bswp    R2, R2
        mvi     R2, lo(100)
        mvi     R0, '/'
        bal     (I_expr)
        bal     (I_div16)
        bal     (I_answer)      ; 300

        mvi     R1, hi(-200)
        bswp    R1, R1
        mvi     R1, lo(-200)
        mvi     R2, hi(100)
        bswp    R2, R2
        mvi     R2, lo(100)
        mvi     R0, '/'
        bal     (I_expr)
        bal     (I_div16)
        bal     (I_answer)      ; -2

        mvi     R1, hi(-30000)
        bswp    R1, R1
        mvi     R1, lo(-30000)
        mvi     R2, hi(-200)
        bswp    R2, R2
        mvi     R2, lo(-200)
        mvi     R0, '/'
        bal     (I_expr)
        bal     (I_div16)
        bal     (I_answer)      ; 150

        mvi     R1, hi(-30000)
        bswp    R1, R1
        mvi     R1, lo(-30000)
        mvi     R2, hi(78)
        bswp    R2, R2
        mvi     R2, lo(78)
        mvi     R0, '/'
        bal     (I_expr)
        bal     (I_div16)
        bal     (I_answer)      ; -384

        mvi     R1, hi(5000)
        bswp    R1, R1
        mvi     R1, lo(5000)
        mvi     R2, hi(4000)
        bswp    R2, R2
        mvi     R2, lo(4000)
        bal     (I_comp)

        mvi     R1, hi(5000)
        bswp    R1, R1
        mvi     R1, lo(5000)
        mvi     R2, hi(5000)
        bswp    R2, R2
        mvi     R2, lo(5000)
        bal     (I_comp)

        mvi     R1, hi(4000)
        bswp    R1, R1
        mvi     R1, lo(4000)
        mvi     R2, hi(5000)
        bswp    R2, R2
        mvi     R2, lo(5000)
        bal     (I_comp)

        mvi     R1, hi(-5000)
        bswp    R1, R1
        mvi     R1, lo(-5000)
        mvi     R2, hi(-4000)
        bswp    R2, R2
        mvi     R2, lo(-4000)
        bal     (I_comp)

        mvi     R1, hi(-5000)
        bswp    R1, R1
        mvi     R1, lo(-5000)
        mvi     R2, hi(-5000)
        bswp    R2, R2
        mvi     R2, lo(-5000)
        bal     (I_comp)

        mvi     R1, hi(-4000)
        bswp    R1, R1
        mvi     R1, lo(-4000)
        mvi     R2, hi(-5000)
        bswp    R2, R2
        mvi     R2, lo(-5000)
        bal     (I_comp)

        mvi     R1, hi(32700)
        bswp    R1, R1
        mvi     R1, lo(32700)
        mvi     R2, hi(32600)
        bswp    R2, R2
        mvi     R2, lo(32600)
        bal     (I_comp)

        mvi     R1, hi(32700)
        bswp    R1, R1
        mvi     R1, lo(32700)
        mvi     R2, hi(32700)
        bswp    R2, R2
        mvi     R2, lo(32700)
        bal     (I_comp)

        mvi     R1, hi(32600)
        bswp    R1, R1
        mvi     R1, lo(32600)
        mvi     R2, hi(32700)
        bswp    R2, R2
        mvi     R2, lo(32700)
        bal     (I_comp)

        mvi     R1, hi(-32700)
        bswp    R1, R1
        mvi     R1, lo(-32700)
        mvi     R2, hi(-32600)
        bswp    R2, R2
        mvi     R2, lo(-32600)
        bal     (I_comp)

        mvi     R1, hi(-32700)
        bswp    R1, R1
        mvi     R1, lo(-32700)
        mvi     R2, hi(-32700)
        bswp    R2, R2
        mvi     R2, lo(-32700)
        bal     (I_comp)

        mvi     R1, hi(-32600)
        bswp    R1, R1
        mvi     R1, lo(-32600)
        mvi     R2, hi(-32700)
        bswp    R2, R2
        mvi     R2, lo(-32700)
        bal     (I_comp)

        mvi     R1, hi(18000)
        bswp    R1, R1
        mvi     R1, lo(18000)
        mvi     R2, hi(-28000)
        bswp    R2, R2
        mvi     R2, lo(-28000)
        bal     (I_comp)

        mvi     R1, hi(-28000)
        bswp    R1, R1
        mvi     R1, lo(-28000)
        mvi     R2, hi(-28000)
        bswp    R2, R2
        mvi     R2, lo(-28000)
        bal     (I_comp)

        mvi     R1, hi(-28000)
        bswp    R1, R1
        mvi     R1, lo(-28000)
        mvi     R2, hi(18000)
        bswp    R2, R2
        mvi     R2, lo(18000)
        bal     (I_comp)

        ret
