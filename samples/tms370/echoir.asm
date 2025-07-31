;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "tms370.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     10F0H
        include "mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

        org     3000H
rx_queue_size:  equ     128
rx_queue:       .block  rx_queue_size

;;; Internal registers
        org     00C0H
stack:  equ     $-1      ; TMS370's SP is pre-increment/post-decrement

        org     VEC_INT2
        .word   isr_int2

        org     VEC_RESET
        .word   initialize

        org     VEC_TRAP15
        .word   VEC_TRAP15

        org     2000H
initialize:
        mov     #stack, B
        ldsp
        movw    #rx_queue, R3
        mov     #rx_queue_size, B
        call    queue_init
;;; initialize ACIA
        mov     #CDS_RESET_gc, ACIA_control ; Master reset
        mov     #RX_INT_TX_NO, ACIA_control
        mov     #2, ACIA+2      ; #INT2 for Rx
        mov     #INT_ENABLE, INT2 ; enable #INT2

loop:
        movw    #rx_queue, R3
        dint                    ; (clear all ST bits)
        call    queue_remove
        eint
        jnc     loop
        tst     A
        jz      halt_to_system
        call    putchar
        cmp     #0DH, A
        jne     loop
        mov     #0AH, A
        call    putchar
        jmp     loop
halt_to_system:
        trap    15

putchar:
        btjz    #TDRE_bm, ACIA_status, putchar
        mov     A, ACIA_data
        rts

        include "queue.inc"

isr_int2:
        and     #~INT_FLAG & 0FFH, INT2 ; clear INT2_FLAG
        btjz    #IRQF_bm, ACIA_status, isr_int2_return
        btjz    #RDRF_bm, ACIA_status, isr_int2_return
        push    A
        mov     ACIA_data, A
        push    R2
        push    R3
        movw    #rx_queue, R3
        call    queue_add
        pop     R3
        pop     R2
        pop     A
isr_int2_return:
        rti
