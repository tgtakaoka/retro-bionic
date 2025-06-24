;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tms7000
        include "tms7000.inc"

        *** MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     >01F0
        include "mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     >2000
rx_queue_size:  equ     128
rx_queue:       bss     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       bss     tx_queue_size

        * Internal registers
        org     >60
        * TMS7000's SP is pre-increment/post-decrement
stack:

        org     VEC_INT3
        data    isr_int3

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
        ** initialize ACIA
        movp    %CDS_RESET_gc, ACIA_control     Master reset
        movp    %RX_INT_TX_NO, ACIA_control
        movp    %3, ACIA+2                      #INT3 for Rx/Tx
        movp    %INT3_F|INT3_E|INT1_F, IOCNT0   enable #INT1 and #INT3

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
        eint
        movd    %tx_queue, R3
        dint
        call    @queue_add
        jnc     putchar
        eint
        movp    %RX_INT_TX_INT, ACIA_control    enable Tx interrupt
        rets

        include "queue.inc"

isr_int3:
        btjzp   %IRQF_bm, ACIA_status, isr_int3_return
        push    A
        push    R2
        push    R3
        btjzp   %RDRF_bm, ACIA_status, isr_tx
        movp    ACIA_data, A
        movd    %rx_queue, R3
        call    @queue_add
isr_tx:
        btjzp   %TDRE_bm, ACIA_status, isr_int3_exit
        movd    %tx_queue, R3
        call    @queue_remove
        jnc     isr_tx_empty
        movp    A, ACIA_data    send character
        jmp     isr_int3_exit
isr_tx_empty:
        movp    %RX_INT_TX_NO, ACIA_control     disable Tx interrupt
isr_int3_exit:
        pop     R3
        pop     R2
        pop     A
isr_int3_return:
        reti
