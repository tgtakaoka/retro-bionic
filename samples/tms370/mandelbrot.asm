;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "tms370.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     10F0H
        include "mc6850.inc"
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  equ     WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        org     3000H
rx_queue_size:  equ     128
rx_queue:       .block  rx_queue_size
tx_queue_size:  equ     128
tx_queue:       .block  tx_queue_size

;;; Internal registers
        org     00004H
RdH:    .block  1
RdL:    .block  1
Rd:     equ     RdL             ; R4:R5
RsH:    .block  1
RsL:    .block  1
Rs:     equ     RsL             ; R6:R7
        .block  1
tmp:    .block  1
        .block  1
vC:     .block  1
        .block  1
vD:     .block  1
        .block  1
vA:     .block  1
        .block  1
vB:     .block  1
        .block  1
vP:     .block  1
        .block  1
vQ:     .block  1
        .block  1
vS:     .block  1
vTH     .block  1
vTL:    .block  1
vT:     equ     vTL
cF:     equ     50
vY:     .block  1
vX:     .block  1
vI:     .block  1

;;; TMS370's SP is pre-increment/post-decrement
        org     00D0H
stack:  equ     $-1

        org     VEC_INT1
        .word   isr_int1

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
        movw    #tx_queue, R3
        mov     #tx_queue_size, B
        call    queue_init
;;; initialize ACIA
        mov     #CDS_RESET_gc, ACIA_control     ; Master reset
        mov     #RX_INT_TX_NO, ACIA_control
        mov     #1, ACIA+2        ; INT1 for Rx/Tx
        mov     #INT_ENABLE, INT1 ; enable falling #INT1
        eint

loop:
        call    mandelbrot
        call    newline
        jmp     loop

;;; Get character
;;; @return A
;;; @return ST.C 0 if no character
;;; @clobber R2:R3
getchar:
        push    st
        movw    #rx_queue, R3
        dint
        call    queue_remove
        jnc     getchar_empty
        pop     st
        setc                    ; ST.C=1
        rts
getchar_empty:
        pop     st              ; ST.C=0
        rts

;;; Put character
;;; @param A
;;; @clobber R2:R3
putspace:
        mov     #' ', A
        jmp     putchar
newline:
        mov     #00DH, A
        call    putchar
        mov     #00AH, A
putchar:
        push    st
        movw    #tx_queue, R3
        dint
        call    queue_add
        jnc     putchar
        mov     #RX_INT_TX_INT, ACIA_control ; enable Tx interrupt
        pop     st
        rts

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

isr_int1:
        and     #~INT_FLAG & 0FFH, INT1 ; clear INT1_FLAG
        btjz    #IRQF_bm, ACIA_status, isr_int1_return
        push    A
        push    R2
        push    R3
        btjz    #RDRF_bm, ACIA_status, isr_tx
        mov     ACIA_data, A
        movw    #rx_queue, R3
        call    queue_add
isr_tx:
        btjz    #TDRE_bm, ACIA_status, isr_int1_exit
        movw    #tx_queue, R3
        call    queue_remove
        jnc     isr_tx_empty
        mov     A, ACIA_data    ; send character
        jmp     isr_int1_exit
isr_tx_empty:
        mov     #RX_INT_TX_NO, ACIA_control ; disable Tx interrupt
isr_int1_exit:
        pop     R3
        pop     R2
        pop     A
isr_int1_return:
        rti
