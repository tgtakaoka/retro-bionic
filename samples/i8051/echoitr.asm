;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8051
        include "i8051.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     0FFF0H
USARTD:         equ     USART+0 ; Receive/Transmit data
USARTS:         equ     USART+1 ; Srtatus register
USARTC:         equ     USART+1 ; Control register
USARTRV:        equ     USART+2 ; Receive interrupt vector (ORG_*)
USARTTV:        equ     USART+3 ; Transmit interrupt vector (ORG_*)
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm
RX_EN_TX_DIS:   equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm

;;; External data memory
        org     2000H
rx_buffer_size: equ     128
rx_buffer:      ds      rx_buffer_size
tx_buffer_size: equ     128
tx_buffer:      ds      tx_buffer_size

;;; Internal data memory
        org     BASE_MEMORY
rx_queue:       ds      queue_work_size
tx_queue:       ds      queue_work_size
stack:          equ     $

        org     ORG_RESET
        ljmp    init
        org     ORG_IE0
        ljmp    isr_intr_rx
        org     ORG_TF0
        ljmp    init
        org     ORG_IE1
        ljmp    isr_intr_tx
        org     ORG_TF1
        ljmp    init
        org     ORG_RITI
        ljmp    init

init:
        mov     SP, #stack-1
        mov     R0, #rx_queue
        mov     R1, #rx_buffer_size
        mov     DPTR, #rx_buffer
        acall   queue_init
        mov     R0, #tx_queue
        mov     R1, #tx_buffer_size
        mov     DPTR, #tx_buffer
        acall   queue_init
init_usart:
        mov     DPTR, #USARTC
        clr     A
        movx    @DPTR, A
        movx    @DPTR, A
        movx    @DPTR, A        ; safest way to sync mode
        mov     a, #CMD_IR_bm
        movx    @DPTR, A        ; reset
        nop
        nop
        mov     a, #ASYNC_MODE
        movx    @DPTR, A
        nop
        nop
        mov     a, #RX_EN_TX_EN
        movx    @DPTR, A
        inc     DPTR
        mov     A, #ORG_IE0
        movx    @DPTR, A        ; enable Rx interrupt using INT0
        setb    IE.EX0          ; enable INT0
        mov     A, #ORG_IE1
        inc     DPTR
        movx    @DPTR, A        ; enable Tx interrupt using INT1
        setb    IE.EX1          ; enable INT1
        setb    IE.EA           ; enable interrupt

receive_loop:
        acall   getchar
        jnc     receive_loop
        jz      halt_to_system
        mov     R3, A           ; save letter
        acall   putchar         ; echo
        acall   putspace
        acall   put_hex8        ; print in hex
        acall   putspace
        acall   put_bin8        ; print in binary
        acall   newline
        sjmp    receive_loop
halt_to_system:
        db      0A5H

;;; Print uint8_t in hex
;;; @param R3 uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        mov     A, #'0'
        acall   putchar
        mov     A, #'x'
        acall   putchar
        mov     A, R3
        swap    A
        acall   put_hex4
        mov     A, R3
put_hex4:
        anl     A, #0FH
        add     A, #-10
        jnc     put_hex8_dec    ; A<10
        add     A, #'A'-('0'+10)
put_hex8_dec:
        add     A, #'0'+10
        sjmp    putchar

;;; Print uint8_t in binary
;;; @param R3 uint8_t value to be printed in binary.
;;; @clobber A R2
put_bin8:
        mov     A, #'0'
        acall   putchar
        mov     A, #'b'
        acall   putchar
        mov     A, R3
        swap    A
        mov     R2, A
        acall   put_bin4
        mov     A, R3
        mov     R2, A
        acall   put_bin4
        ret
put_bin4:
        mov     A, R2
        rr      A
        rr      A
        mov     R2, A
        acall   put_bin2
        mov     A, R2
        rl      A
        rl      A
        mov     R2, A
put_bin2:
        mov     A, R2
        rr      A
        acall   put_bin1
        mov     A, R2
put_bin1:
        anl     A, #1
        add     A, #'0'
        sjmp    putchar

;;; Get character
;;; @return A
;;; @return PSW.C 0 if no character
;;; @clobber DPTR
getchar:
        mov     A, R0
        push    ACC             ; save R0
        mov     R0, #rx_queue
        clr     IE.EA
        acall   queue_remove
        setb    IE.EA
        xch     A, R0           ; R0=character
        pop     ACC
        xch     A, R0           ; restore R0
        ret

;;; Put space
;;; @clobber A
putspace:
        mov     A, #' '
        sjmp    putchar

;;; Put newline
;;; @clobber A
newline:
        mov     A, #0DH
        acall   putchar
        mov     A, #0AH

;;; Put character
;;; @param A
;;; @clobber DPTR A
putchar:
        xch     A, R0           ; save character
        push    ACC             ; save R0
        mov     A, R0           ; restore character
putchar_loop:
        mov     R0, #tx_queue
        clr     IE.EA
        acall   queue_add
        setb    IE.EA
        jnc     putchar_loop    ; branch if queue is full
        mov     DPTR, #USARTC
        mov     A, #RX_EN_TX_EN ; enable Tx
        movx    @DPTR, A
        pop     ACC
        mov     R0, A           ; restore R0
        ret

        include "queue.inc"

isr_intr_rx:
        push    PSW
        clr     PSW.RS1
        setb    PSW.RS0         ; select BANK 1
        mov     R1, DPL
        mov     R2, DPH         ; save DPTR
        mov     R3, A           ; save A
        mov     DPTR, #USARTS
        movx    A, @DPTR
        jnb     ACC.ST_RxRDY_bp, isr_intr_exit
        dec     DPL
        movx    A, @DPTR
        mov     R0, #rx_queue
        acall   queue_add
isr_intr_exit:
        mov     A, R3           ; restore A
        mov     DPH, R2
        mov     DPL, R1         ; restore DPTR
        pop     PSW
        reti

isr_intr_tx:
        push    PSW
        clr     PSW.RS1
        setb    PSW.RS0         ; select BANK 1
        mov     R1, DPL
        mov     R2, DPH         ; save DPTR
        mov     R3, A           ; save A
        mov     DPTR, #USARTS
        movx    A, @DPTR
        jnb     ACC.ST_TxRDY_bp, isr_intr_exit
        mov     R0, #tx_queue
        acall   queue_remove
        jnc     isr_intr_tx_empty
        mov     DPTR, #USARTD
        movx    @DPTR, A        ; send character
        sjmp    isr_intr_exit
isr_intr_tx_empty:
        mov     DPTR, #USARTC
        mov     A, #RX_EN_TX_DIS
        movx    @DPTR, A        ; disable Tx
        sjmp    isr_intr_exit

        end
