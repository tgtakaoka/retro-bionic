;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8048
        include "i8048.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     0FCH
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
        org     00H
rx_queue_size:  equ     8
rx_queue_buf:   ds      rx_queue_size
tx_queue_size:  equ     56
tx_queue_buf:   ds      tx_queue_size
        org     USART
;;;  Software stack; pre-decrement, post-increment pointed by R1 on
;;;  external data memory
stack:          equ     $

;;; Internal data memory
        org     BASE_MEMORY
rx_queue:       ds      queue_work_size
tx_queue:       ds      queue_work_size

        org     ORG_RESET
        jmp     init
        org     ORG_INT
        jmp     isr_intr
init:
        mov     R1, #stack
        mov     R0, #rx_queue
        mov     R2, #rx_queue_buf
        mov     A, #rx_queue_size
        call    queue_init
        mov     R0, #tx_queue
        mov     R2, #tx_queue_buf
        mov     A, #tx_queue_size
        call    queue_init
init_usart:
        mov     R0, #USARTC
        clr     A
        movx    @R0, A
        movx    @R0, A
        movx    @R0, A          ; safest way to sync mode
        mov     a, #CMD_IR_bm
        movx    @R0, A          ; reset
        nop
        nop
        mov     a, #ASYNC_MODE
        movx    @R0, A
        nop
        nop
        mov     a, #RX_EN_TX_EN
        movx    @R0, A
        mov     a, #ORG_INT
        mov     R0, #USARTRV
        movx    @R0, A          ; enable RxRDY interrupt
        inc     R0
        movx    @R0, A          ; enable TxRDY interrupt
        en      I

receive_loop:
        call    getchar
        jnc     receive_loop
        jz      halt_to_system
        mov     R3, A           ; save letter
        call    putchar         ; echo
        call    putspace
        call    put_hex8        ; print in hex
        call    putspace
        call    put_bin8        ; print in binary
        call    newline
        jmp     receive_loop
halt_to_system:
        db      01H

;;; Print uint8_t in hex
;;; @param R3 uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        mov     A, #'0'
        call    putchar
        mov     A, #'x'
        call    putchar
        mov     A, R3
        swap    A
        call    put_hex4
        mov     A, R3
put_hex4:
        anl     A, #0FH
        add     A, #-10
        jnc     put_hex8_dec    ; A<10
        add     A, #'A'-('0'+10)
put_hex8_dec:
        add     A, #'0'+10
        jmp     putchar

;;; Print uint8_t in binary
;;; @param R3 uint8_t value to be printed in binary.
;;; @clobber A R2
put_bin8:
        mov     A, #'0'
        call    putchar
        mov     A, #'b'
        call    putchar
        mov     A, R3
        swap    A
        mov     R2, A
        call    put_bin4
        mov     A, R3
        mov     R2, A
        call    put_bin4
        ret
put_bin4:
        mov     A, R2
        rr      A
        rr      A
        mov     R2, A
        call    put_bin2
        mov     A, R2
        rl      A
        rl      A
        mov     R2, A
put_bin2:
        mov     A, R2
        rr      A
        call    put_bin1
        mov     A, R2
put_bin1:
        anl     A, #1
        add     A, #'0'
        jmp     putchar
        
;;; Get character
;;; @return A
;;; @return PSW.C 0 if no character
getchar:
        mov     A, R0
        dec     R1
        movx    @R1, A          ; save R0
        mov     R0, #rx_queue
        dis     I
        call    queue_remove
        en      I
        xch     A, R0           ; save character
        movx    A, @R1          ; restore R0
        inc     R1
        xch     A, R0           ; restore character
        ret

;;; Put space
;;; @clobber A
putspace:
        mov     A, #' '
        jmp     putchar

;;; Put newline
;;; @clobber A
newline:
        mov     A, #0DH
        call    putchar
        mov     A, #0AH

;;; Put character
;;; @param A
;;; @clobber A
putchar:
        xch     A, R0           ; save character
        dec     R1
        movx    @R1, A          ; save R0
        mov     A, R0           ; restore character
putchar_loop:
        mov     R0, #tx_queue
        dis     I
        call    queue_add
        en      I
        jnc     putchar_loop    ; branch if queue is full
        mov     R0, #USARTC
        mov     A, #RX_EN_TX_EN ; enable Tx
        movx    @R0, A
        movx    A, @R1
        inc     R1
        mov     R0, A           ; restore R0
        ret

        include "queue.inc"

isr_intr:
        sel     RB1             ; switch context
        mov     R2, A           ; save A
        mov     R0, #USARTS
        movx    A, @R0
        cpl     A
        jb      ST_RxRDY_bp, isr_intr_tx
        mov     R0, #USARTD
        movx    A, @R0
        mov     R0, #rx_queue
        call    queue_add
isr_intr_tx:    
        mov     R0, #USARTS
        movx    A, @R0
        cpl     A
        jb      ST_TxRDY_bp, isr_intr_exit
        mov     R0, #tx_queue
        call    queue_remove
        jnc     isr_intr_empty
        mov     R0, #USARTD
        movx    @R0, A          ; send character
        xrl     A, #'&'
        jnz     isr_intr_exit
        db      01H
isr_intr_exit:
        mov     A, R2           ; restore A
        retr
isr_intr_empty:
        mov     R0, #USARTC
        mov     A, #RX_EN_TX_DIS
        movx    @R0, A          ; disable Tx
        mov     A, R2           ; restore A
        retr

        end
