;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tms7000
        include "tms7000.inc"

        *** MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     >01F0
        include "mc6850.inc"

        org     VEC_RESET
        data    initialize

        org     >1000
initialize:
        movp    %CDS_RESET_gc, ACIA_control     Master reset
        movp    %WSB_8N1_gc, ACIA_control       8 bits + No Parity + 1 Stop Bits
        *                                       Transmit, Receive interrupts disabled
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
        btjzp   %RDRF_bm, ACIA_status, getchar
        movp    ACIA_data, A
        rets

putchar:
        btjzp   %TDRE_bm, ACIA_status, putchar
        movp    A, ACIA_data
        rets

        *** Local Variables:
        *** mode: asm
        *** mode: flyspell-prog
        *** comment-start: "*"
        *** End:
