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
        org     >0004
RdH:    bss     1
RdL:    bss     1
Rd:     equ     RdL             ; R4:R5
RsH:    bss     1
RsL:    bss     1
Rs:     equ     RsL             ; R6:R7
        bss     1
tmp:    bss     1
        bss     1
vC:     bss     1
        bss     1
vD:     bss     1
        bss     1
vA:     bss     1
        bss     1
vB:     bss     1
        bss     1
vP:     bss     1
        bss     1
vQ:     bss     1
        bss     1
vS:     bss     1
vTH     bss     1
vTL:    bss     1
vT:     equ     vTL
cF:     equ     50
vY:     bss     1
vX:     bss     1
vI:     bss     1
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
        movd    %tx_queue, R3
        mov     %tx_queue_size, B
        call    @queue_init
        ** initialize ACIA
        movp    %CDS_RESET_gc, ACIA_control     Master reset
        movp    %RX_INT_TX_NO, ACIA_control
        movp    %1, ACIA+2                      #INT1 for Rx/Tx
        movp    %INT3_F|INT1_E|INT1_F, IOCNT0   enable #INT1 and #INT1

loop:
        call    @mandelbrot
        call    @newline
        jmp     loop

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
putspace:
        mov     %' ', A
        jmp     putchar
newline:
        mov     %>0D, A
        call    @putchar
        mov     %>0A, A
putchar:
        eint
        movd    %tx_queue, R3
        dint
        call    @queue_add
        jnc     putchar
        eint
        movp    %RX_INT_TX_INT, ACIA_control    enable Tx interrupt
        rets

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

isr_int1:
        btjzp   %IRQF_bm, ACIA_status, isr_int1_return
        push    A
        push    R2
        push    R3
        btjzp   %RDRF_bm, ACIA_status, isr_tx
        movp    ACIA_data, A
        movd    %rx_queue, R3
        call    @queue_add
isr_int1_exit:
        pop     R3
        pop     R2
        pop     A
isr_int1_return:
        reti
isr_tx:
        btjzp   %TDRE_bm, ACIA_status, isr_int1_exit
        movd    %tx_queue, R3
        call    @queue_remove
        jnc     isr_tx_empty
        movp    A, ACIA_data    send character
        jmp     isr_int1_exit
isr_tx_empty:
        movp    %RX_INT_TX_NO, ACIA_control     disable Tx interrupt
        jmp     isr_int1_exit
