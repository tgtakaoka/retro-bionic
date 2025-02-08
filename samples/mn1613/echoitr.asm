*** -*- mode: asm; mode: flyspell-prog; -*-
        include "mn1613.inc"
*** i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     X'80'
USARTD:         equ     USART+0 Data register
USARTS:         equ     USART+1 Status register
USARTC:         equ     USART+1 Control register
USARTRV:        equ     USART+2 Receive interrupt vector (NPSWx)
USARTTV:        equ     USART+3 Transmit interrupt vector (NPSWx)
        include "i8251.inc"
* Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
* RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm
RX_EN_TX_DIS:   equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm

        loc     RESET_PSW
        dc      X'0000'         STR: Disable interrupt
        dc      initialize      IC

        loc     NPSW1
        dc      X'0000'
        dc      irq1_isr

        loc     NPSW2
        dc      X'0000'
        dc      irq2_isr

        loc     X'2000'
rx_queue_size:  equ     128
rx_queue:
        ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:
        ds      tx_queue_size

        loc     X'0200'
stack:  equ     *-1
initialize:
        mvwi    SP, stack
        mvwi    R0, rx_queue_size
        mvwi    X0, rx_queue
        bal     queue_init
        mvwi    R0, tx_queue_size
        mvwi    X0, tx_queue
        bal     queue_init
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
        mvi     R0, RX_EN_TX_DIS
        wt      R0, USARTC      Enable Rx, Disable Tx
        mvi     R0, OPSW1+1     Use IRQ1 for Rx interrupt
        wt      R0, USARTRV
        mvi     R0, OPSW2+1     Use IRQ2 for Tx interrupt
        wt      R0, USARTTV
        ret

loop:   bal     getchar
        or      R0, R0, nz      Skip if R0!=0
        h                       Halt to system
        mv      R1, R0
        bal     putchar
        mvi     R0, ' '
        bal     putchar
        bal     put_hex8
        mvi     R0, ' '
        bal     putchar
        bal     put_bin8
        bal     newline
        b       loop

getchar:
        push    X0
        mvwi    X0, rx_queue
getchar_wait:
        rbit    STR, STR_IRQ1           Disable IRQ1
        bal     queue_remove
        sbit    STR, STR_IRQ1, enz      Enable IRQ1, skip if E=1
        b       getchar_wait
        pop     X0
        ret

put_hex8:
        mvi     R0, '0'
        bal     putchar
        mvi     R0, 'x'
        bal     putchar
        dswp    R0, R1
        bswp    R0, R0
        bal     put_hex4
        mv      R0, R1
put_hex4:
        push    R1
        andi    R0, X'0F'
        cbi     R0, 10, lpz     skip if R0>=10
        b       put_hex4_dec
        si      R0, 10
        awi     R0, 'A', skp
put_hex4_dec:
        awi     R0, '0'
        pop     R1
        b       putchar

put_bin8:
        mvi     R0, '0'
        bal     putchar
        mvi     R0, 'b'
        bal     putchar
        bal     put_bin4
put_bin4:
        bal     put_bin2
put_bin2:
        bal     put_bin1
put_bin1:
        sl      R1, re          R1<<=1
        mvi     R0, '0'
        tbit    R1, 7, z        skip if lsb(hi(R1))=0
        mvi     R0, '1'
        b       putchar

newline:
        mvi     R0, X'0D'
        bal     putchar
        mvi     R0, X'0A'
putchar:
        push    R0
        push    X0
        mvwi    X0, tx_queue
putchar_retry:
        rbit    STR, STR_IRQ2           Disable IRQ2
        bal     queue_add
        sbit    STR, STR_IRQ2, enz      Enable IRQ2, skip if E=1
        b       putchar_retry
        mvi     R0, RX_EN_TX_EN
        wt      R0, USARTC              Enable Rx/Tx
        pop     X0
        pop     R0
        ret

        include "queue.inc"

irq1_isr:
        push    R0
        push    X0
        rd      R0, USARTS
        tbit    R0, ST_RxRDY_bp, nz     Skip if RxRDY=1
        b       irq1_isr_exit
        rd      R0, USARTD              Receive character
        mvwi    X0, rx_queue
        bal     queue_add               Add to Rx queue
irq1_isr_exit:
        pop     X0
        pop     R0
        lpsw    1               Return from interrupt

irq2_isr:
        push    R0
        push    X0
        rd      R0, USARTS
        tbit    R0, ST_TxRDY_bp, nz     Skip if TxRDY=1
        b       irq2_isr_exit
        mvwi    X0, tx_queue
        bal     queue_remove    Remove from Tx queue
        mv      R0, R0, enz     Skip if ST.E=1
        b       irq2_isr_empty
        wt      R0, USARTD      Transmit charater
irq2_isr_exit:
        pop     X0
        pop     R0
        lpsw    2               Return from interrupt
irq2_isr_empty:
        mvi     R0, RX_EN_TX_DIS
        wt      R0, USARTC      Disable Tx
        b       irq2_isr_exit
