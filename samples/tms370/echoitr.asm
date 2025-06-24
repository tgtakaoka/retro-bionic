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
        org     00D0H
stack:  equ     $-1     ; TMS370's SP is pre-increment/post-decrement

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
        call    getchar
        jnc     loop
        tst     A
        jz      halt_to_system
        mov     A, B
        call    putchar        echo
        call    putspace
        call    put_hex8       print in hex
        call    putspace
        call    put_bin8       print in binary
        call    newline
        jmp     loop
halt_to_system:
        trap    15

;;; Put sapce
;;; @clobber A
putspace:
        mov     #' ', A
        jmp     putchar

;;; Put newline
;;; @clobber A
newline:
        mov     #0DH, A
        call    putchar
        mov     #0AH, A
        jmp     putchar

;;; Print uint8_t in hex
;;; @param B uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        mov     #'0', A
        call    putchar
        mov     #'x', A
        call    putchar
        mov     B, A
        swap    A
        call    put_hex4
        mov     B, A
put_hex4:
        and     #0FH, A
        cmp     #10, A
        jl      put_hex8_dec
        add     #'A'-10-'0', A
put_hex8_dec:
        add     #'0', A
        jmp     putchar

;;; Print uint8_t in binary
;;; @param B uint8_t value to be printed in binary.
;;; @clobber A B
put_bin8:
        mov     #'0', A
        call    putchar
        mov     #'b', A
        call    putchar
        call    put_bin4
put_bin4:
        call    put_bin2
put_bin2:
        call    put_bin1
put_bin1:
        mov     #'0', A
        rlc     B
        jnc     putchar         ; MSB=0
        inc     A               ; MSB=1
        jmp     putchar

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
putchar:
        movw    #tx_queue, R3
        dint
        call    queue_add
        jnc     putchar
        mov     #RX_INT_TX_INT, ACIA_control ; enable Tx interrupt
        eint
        rts

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
