;;; -*- mode: asm; mode: flyspell-prog; -*-
        org     VEC_RESET
        data    initialize

        org     >1000
initialize:
        orp     %?1000, BPORT                   Pass through TXD (PB3=1)
        movp    %0, SMODE                       Select SCTL0
        movp    %UR_bm, SCTL0                   Select SMODE by reset
        movp    %CMODE_bm|CHAR8_bm|ASYNC_bm, SMODE       8 bits asynchronous
        movp    %RXEN_bm|TXEN_bm, SCTL0         Rx/Tx enable
        movp    %12, T3DATA                     9600 bps (TR)
        movp    %CLK_bm|T3RUN_bm, SCTL1         Enable internal clock, PR=0

loop:   call    @getchar
        tsta
        jeq     halt_to_system
echo:   call    @putchar
        cmp     %>0D, A
        jne     loop
        mov     %>0A, A
        jmp     echo
halt_to_system:
        idle

getchar:
        btjzp   %RXRDY_bm, SSTAT, getchar
        movp    RXBUF, A
        rets

putchar:
        btjzp   %TXRDY_bm, SSTAT, putchar
        movp    A, TXBUF
        rets
