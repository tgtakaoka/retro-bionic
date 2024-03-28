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
print_uint16_buf:
        ds      8
        org     USART
;;;  Software stack; pre-decrement, post-increment pointed by R1 on
;;;  external data memory
stack:          equ     $

;;; Internal data memory
        org     BASE_MEMORY
rx_queue:       ds      queue_work_size
tx_queue:       ds      queue_work_size
;;; Work area for mandelbrot.inc
tmp:    ds      2
c229:   ds      2
c416:   ds      2
c100:   ds      2
vF:     ds      2
vC:     ds      2
vD:     ds      2
vA:     ds      2
vB:     ds      2
vP:     ds      2
vQ:     ds      2
vS:     ds      2
vT:     ds      2
vY:     ds      2
vX:     ds      2
vI:     ds      2

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

loop:
        call    mandelbrot
        call    newline
        jmp     loop

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

;;; Put newline
;;; @clobber A
newline:
        mov     A, #0DH
        call    putchar
        mov     A, #0AH
        jmp     putchar

;;; Put space
;;; @clobber A
putspace:
        mov     A, #' '

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
isr_intr_exit:
        mov     A, R2           ; restore A
        retr
isr_intr_empty:
        mov     R0, #USARTC
        mov     A, #RX_EN_TX_DIS
        movx    @R0, A          ; disable Tx
        mov     A, R2           ; restore A
        retr

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

        end
