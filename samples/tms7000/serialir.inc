;;; -*- mode: asm; mode: flyspell-prog; -*-
        org     >2000
rx_queue_size:  equ     128
rx_queue:       bss     rx_queue_size

        * Internal registers
        org     >60
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
        ** initialize Serial
        orp     %?1000, BPORT                   Pass through TXD (PB3=1)
        movp    %0, SMODE                       Select SCTL0
        movp    %UR_bm, SCTL0                   Select SMODE by reset
        movp    %CMODE_bm|CHAR8_bm|ASYNC_bm, SMODE       8 bits asynchronous
        movp    %RXEN_bm|TXEN_bm, SCTL0         Rx/Tx enable
        movp    %12, T3DATA                     9600 bps (TR)
        movp    %CLK_bm|T3RUN_bm, SCTL1         Enable internal clock, PR=0
        movp    %INT4_E|INT4_F, IOCNT1          clear/enable INT4

loop:
        eint
        movd    %rx_queue, R3
        dint                    (clear all ST bits)
        call    @queue_remove
        jnc     loop
        eint
        tsta
        jz      halt_to_system
        call    @putchar
        cmp     %>0D, A
        jne     loop
        mov     %>0A, A
        call    @putchar
        jmp     loop
halt_to_system:
        idle

putchar:
        btjzp   %TXRDY_bm, SSTAT, putchar
        movp    A, TXBUF
        rets

        include "queue.inc"

isr_int4:
        btjzp   %RXRDY_bm, SSTAT, isr_int4_return
        push    A
        movp    RXBUF, A
        push    R2
        push    R3
        movd    %rx_queue, R3
        call    @queue_add
        pop     R3
        pop     R2
        pop     A
isr_int4_return:
        reti
