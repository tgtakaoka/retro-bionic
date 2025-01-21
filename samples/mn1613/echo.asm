*** -*- mode: asm; mode: flyspell-prog; -*-
        include "mn1613.inc"
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


        loc     X'0200'
stack:  equ     *-1
initialize:
        mvwi    SP, stack
        bal     init_usart
        b       loop

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

loop:   bal     getchar
        or      R0, R0, nz      Skip if R0!=0
        h                       Halt to system
echo:   bal     putchar
        cbi     R0, x'0D', e    Skip if R0=CR
        b       loop
        mvi     R0, x'0A'       NL
        b       echo

getchar:
        rd      R0, USARTS
        tbit    R0, ST_RxRDY_bp, nz     Skip if RxRDY=1
        b       getchar
        rd      R0, USARTD
        ret

putchar:
        push    R0
putchar_wait:
        rd      R0, USARTS
        tbit    R0, ST_TxRDY_bp, nz     Skip if TxRDY=1
        b       putchar_wait
        pop     R0
        wt      R0, USARTD
        ret
