;;; -*- mode: asm; mode: flyspell-prog; -*-
        org     >2000
rx_queue_size:  equ     128
rx_queue:       bss     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       bss     tx_queue_size

        * Internal registers
        org     >10
tx_enable:      bss     1
        * TMS7000's SP is pre-increment/post-decrement
stack:

        org     VEC_INT4
        data    isr_int4

        org     VEC_RESET
        data    initialize

        org     >1000
initialize:
        mov     %stack, B
        ldsp
        movd    %rx_queue, R3
        mov     %rx_queue_size, B
        call    @queue_init
        movd    %tx_queue, R3
        mov     %tx_queue_size, B
        call    @queue_init
        ** initialize Serial
        orp     %?1000, BPORT                   Pass through TXD (PB3=1)
        movp    %0, SMODE                       Select SCTL0
        movp    %UR_bm, SCTL0                   Select SMODE by reset
        movp    %CMODE_bm|CHAR8_bm|ASYNC_bm, SMODE       8 bits asynchronous
        clr     tx_enable                       Tx disable
        movp    %RXEN_bm, SCTL0                 Rx enable
        movp    %12, T3DATA                     9600 bps (TR)
        movp    %CLK_bm|T3RUN_bm, SCTL1         Enable internal clock, PR=0
        movp    %INT4_F|INT4_E, IOCNT1          clear/enable INT4

loop:
        call    @getchar
        jnc     loop
        tsta
        jz      halt_to_system
        mov     A, B
        call    @putchar        echo
        call    @putspace
        call    @put_hex8       print in hex
        call    @putspace
        call    @put_bin8       print in binary
        call    @newline
        jmp     loop
halt_to_system:
        idle

        *** Put sapce
        *** @clobber A
putspace:
        mov     %' ', A
        jmp     putchar

        *** Put newline
        *** @clobber A
newline:
        mov     %>0D, A
        call    @putchar
        mov     %>0A, A
        jmp     putchar

        *** Print uint8_t in hex
        *** @param B uint8_t value to be printed in hex.
        *** @clobber A
put_hex8:
        mov     %'0', A
        call    @putchar
        mov     %'x', A
        call    @putchar
        mov     B, A
        swap    A
        call    @put_hex4
        mov     B, A
put_hex4:
        and     %>0F, A
        cmp     %10, A
        jl      put_hex8_dec
        add     %'A'-10-'0', A
put_hex8_dec:
        add     %'0', A
        jmp     putchar

        *** Print uint8_t in binary
        *** @param B uint8_t value to be printed in binary.
        *** @clobber A B
put_bin8:
        mov     %'0', A
        call    @putchar
        mov     %'b', A
        call    @putchar
        call    @put_bin4
put_bin4:
        call    @put_bin2
put_bin2:
        call    @put_bin1
put_bin1:
        mov     %'0', A
        rlc     B
        jnc     putchar         ; MSB=0
        inc     A               ; MSB=1
        jmp     putchar

        *** Get character
        *** @return A
        *** @return ST.C 0 if no character
        *** @clobber R2:R3
getchar:
        movd    %rx_queue, R3
        dint
        call    @queue_remove
        jnc     getchar_empty
        eint                    ST.C=1
        rets
getchar_empty:
        eint                    ST.C=1
        clrc
        rets

        *** Put character
        *** @param A
        *** @clobber R2:R3
putchar:
        btjo    %1, tx_enable, putchar_add
        or      %1, tx_enable           enable Tx
        movp    %RXEN_bm|TXEN_bm, SCTL0
        movp    A, TXBUF
        rets
putchar_add:
        eint
        movd    %tx_queue, R3
        dint
        call    @queue_add
        jnc     putchar_add
        eint
        rets

        include "queue.inc"

isr_int4:
        push    A
        push    R2
        push    R3
        btjzp   %RXRDY_bm, SSTAT, isr_tx
        movp    RXBUF, A
        movd    %rx_queue, R3
        call    @queue_add
isr_tx:
        btjzp   %TXRDY_bm, SSTAT, isr_int4_exit
        movd    %tx_queue, R3
        call    @queue_remove
        jnc     isr_tx_empty
        movp    A, TXBUF        send character
isr_int4_exit:
        pop     R3
        pop     R2
        pop     A
        reti
isr_tx_empty:
        clr     tx_enable       disable Tx
        movp    %RXEN_bm, SCTL0
        jmp     isr_int4_exit
