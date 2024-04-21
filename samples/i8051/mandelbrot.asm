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
rx_buffer:
        ds      rx_buffer_size
tx_buffer_size: equ     128
tx_buffer:
        ds      tx_buffer_size
print_uint16_buf:
        ds      8

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
vY:     ds      1
vX:     ds      1
vI:     ds      1
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

loop:
        acall   mandelbrot
        acall   newline
        sjmp    loop

;;; Get character
;;; @return A
;;; @return PSW.C 0 if no character
getchar:
        push    DPH
        push    DPL
        mov     A, R0
        push    ACC             ; save R0
        mov     R0, #rx_queue
        clr     IE.EA
        acall   queue_remove
        setb    IE.EA
        xch     A, R0           ; R0=character
        pop     ACC
        xch     A, R0           ; restore R0
        pop     DPL
        pop     DPH
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
        push    DPH
        push    DPL             ; save DPTR
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
        pop     DPL
        pop     DPH             ; restore DPTR
        ret

        include "mandelbrot.inc"
        include "arith.inc"
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
