;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tms7000
        include "tms7000.inc"

        *** MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     >01F0
        include "mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

        org     >2000
rx_queue_size:  equ     128
rx_queue:       bss     rx_queue_size

        * Internal registers
        org     >60
        * TMS7000's SP is pre-increment/post-decrement
stack:

        org     VEC_INT1
        data    isr_int1

        org     VEC_RESET
        data    initialize

        org     >1000
initialize:
        mov     %stack, B
        ldsp
        movd    %rx_queue, R3
        mov     %rx_queue_size, B
        call    @queue_init
        ** initialize ACIA
        movp    %CDS_RESET_gc, ACIA_control     Master reset
        movp    %RX_INT_TX_NO, ACIA_control
        movp    %1, ACIA+2                      #INT1 for Rx
        movp    %INT3_F|INT1_F|INT1_E, IOCNT0   enable #INT1

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
        btjzp   %TDRE_bm, ACIA_status, putchar
        movp    A, ACIA_data
        rets

        include "queue.inc"

isr_int1:
        btjzp   %IRQF_bm, ACIA_status, isr_int1_return
        btjzp   %RDRF_bm, ACIA_status, isr_int1_return
        push    A
        movp    ACIA_data, A
        push    R2
        push    R3
        movd    %rx_queue, R3
        call    @queue_add
        pop     R3
        pop     R2
        pop     A
isr_int1_return:
        reti
