;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "i8096.inc"
        include "usart.inc"

        org     3000H
rx_queue_size:  equ     128
rx_queue:       dsb     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       dsb     tx_queue_size

        org     1000H
stack:  equ     $

        org     VEC_EXTINT
        dcw     isr_intr

        org     1AH
A:      dsb     1
B:      dsb     1
HL:     dsw     1
DE:     dsw     1

        org     ORG_RESET
init:
        ld      SP, #stack
        ldb     INT_MASK, INT_EXTINT ; enable EXTINT

        ld      HL, #rx_queue
        ldb     B, #rx_queue_size
        scall   queue_init
        ld      HL, #tx_queue
        ldb     B, #tx_queue_size
        scall   queue_init
init_usart:
        stb     ZERO, USARTC
        stb     ZERO, USARTC
        stb     ZERO, USARTC    ; safest way to sync mode
        ldb     A, #CMD_IR_bm
        stb     A, USARTC       ; reset
        nop
        nop
        ldb     A, #ASYNC_MODE
        stb     A, USARTC
        nop
        nop
        ldb     A, #RX_EN_TX_EN
        stb     A, USARTC
        ldb     A, #1
        stb     A, USARTRV      ; enable receive interrupt
        stb     A, USARTTV      ; enable transmit interrupt
        ei

receive_loop:
        scall   getchar
        jnc     receive_loop
        or      A, A
        je      halt_to_system
echo_back:
        ldb     B, A
        scall   putchar         ; echo
        ldb     A, #' '         ; space
        scall   putchar
        scall   put_hex8        ; print in hex
        ldb     A, #' '         ; space
        scall   putchar
        scall   put_bin8        ; print in binary
        scall   newline
        sjmp    receive_loop

halt_to_system:
        ld      A, #VEC_TRAP
        st      A, VEC_TRAP     ; halt to system
        trap

;;; Print uint8_t in hex
;;; @param B uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        ldb     A, #'0'
        scall   putchar
        ldb     A, #'x'
        scall   putchar
        ldb     A, B
        shrb    A, 4
        scall   put_hex4
        ldb     A, B
put_hex4:
        andb    A, #0FH
        cmpb    A, #10
        jlt     put_hex4_ascii
        addb    A, #'A'-'0'-10
put_hex4_ascii:
        addb    A, #'0'
        sjmp    putchar

;;; Print uint8_t in binary
;;; @param B uint8_t value to be printed in binary.
put_bin8:
        push    A               ; push A,B
        ldb     A, #'0'
        scall   putchar
        ldb     A, #'b'
        scall   putchar
        ldb     A, B
        scall   put_bin4
        scall   put_bin4
        pop     A
        ret
put_bin4:
        scall   put_bin2
put_bin2:
        scall   put_bin1
put_bin1:
        shlb    B, 1            ; F.C=MSB
        ldb     A, #'0'
        jnc     putchar         ; F.0=1
        incb    A               ; F.C=1
        sjmp    putchar

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
getchar:
        push    HL
        ld      HL, #rx_queue
        di
        scall   queue_remove
        ei
        pop     HL
        ret

;;; Put newline
;;; @clobber A
newline:
        ldb     A, #0DH
        scall   putchar
        ldb    A, #0AH

;;; Put character
;;; @param A
putchar:
        push    A
        push    HL
        ld      HL, #tx_queue
putchar_retry:
        di
        scall   queue_add
        ei
        jnc     putchar_retry   ; branch if queue is full
        pop     HL
        ldb     A, #RX_EN_TX_EN ; enable Tx
        stb     A, USARTC
        pop     A
        ret

        include "queue.inc"

isr_intr:
        pushf
        push    A               ; push A,B
        push    HL
        ldb     A, USARTS
        jbc     A, ST_RxRDY_bp, isr_intr_tx
        ldb     A, USARTD       ; receive character
        ld      HL, #rx_queue
        scall   queue_add
isr_intr_tx
        ldb     A, USARTS
        jbc     A, ST_TxRDY_bp, isr_intr_exit
        ld      HL, #tx_queue
        scall   queue_remove
        jnc     isr_intr_send_empty
        stb     A, USARTD       ; send character
isr_intr_exit:
        pop     HL
        pop     A
        popf
        ret
isr_intr_send_empty:
        ldb     A, #RX_EN_TX_DIS
        stb     A, USARTC       ; disable Tx
        sjmp    isr_intr_exit

        end
