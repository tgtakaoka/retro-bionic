;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "cp1600.inc"
;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     X'FFF0'
USARTD:         equ     USART+0 ; Data register
USARTS:         equ     USART+1 ; Status register
USARTC:         equ     USART+1 ; Control register
USARTRV:        equ     USART+2 ; Receive interrupt vector
USARTTV:        equ     USART+3 ; Transmit interrupt vector
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm|CMD_TxEN_bm
RX_EN_TX_DIS:   equ     CMD_RTS_bm|CMD_DTR_bm|CMD_ER_bm|CMD_RxEN_bm

;;; The interrupt name the USART is given is its vector.
        org     X'0010'
USART_VEC:
        jd      isr_usart       ; Disable interrupts with the jump

        org     X'2000'
rx_queue_size:  equ     128
rx_queue:
        res     rx_queue_size
tx_queue_size:  equ     128
tx_queue:
        res     tx_queue_size

        org     X'0200'
stack:

        org     RESET_PC
initialize:
        mvii    stack, r6
        mvii    rx_queue, r3
        mvii    rx_queue_size, r0
        jsr     r5, queue_init
        mvii    tx_queue, r3
        mvii    tx_queue_size, r0
        jsr     r5, queue_init
        jsr     r5, init_usart
        eis
loop:   jsr     r5, getchar
        tstr    r0
        beq     halt_to_system  ; halt on NUL
        movr    r0, r1
        jsr     r5, putchar
        jsr     r5, putspace
        jsr     r5, put_hex8
        jsr     r5, putspace
        jsr     r5, put_bin8
        jsr     r5, newline
        b       loop
halt_to_system:
        hlt
        b       halt_to_system  ; HLT stops after this

init_usart:
        clrr    r0
        mvo     r0, USARTC
        mvo     r0, USARTC
        mvo     r0, USARTC      ; Safest way to sync mode
        mvii    CMD_IR_bm, r0
        mvo     r0, USARTC      ; Reset
        nop
        nop
        mvii    ASYNC_MODE, r0
        mvo     r0, USARTC
        nop
        nop
        mvii    RX_EN_TX_DIS, r0
        mvo     r0, USARTC      ; Enable Rx, Disable Tx
        ;; Rx and Tx share #INTRM, so share the vector too.
        mvii    USART_VEC, r0
        mvo     r0, USARTRV     ; Enable Rx interrupt
        mvo     r0, USARTTV     ; Enable Tx interrupt
        jr      r5

;;; @return R0 received char
getchar:
        pshr    r5
        pshr    r3
        mvii    rx_queue, r3
getchar_wait:
        jsrd    r5, queue_remove
        eis
        bnc     getchar_wait    ; branch if queue is empty
        pulr    r3
        pulr    r7

;;; Print "0xXX"
;;; @param R1 value
;;; @clobber R0
put_hex8:
        pshr    r5
        mvii    '0', r0
        jsr     r5, putchar
        mvii    'x', r0
        jsr     r5, putchar
        movr    r1, r0
        slr     r0, 2
        slr     r0, 2
        jsr     r5, put_hex4
        movr    r1, r0
        pulr    r5
put_hex4:
        andi    X'0F', r0
        cmpi    10, r0
        blt     put_hex4_dec    ; branch if R0<10
        addi    'A'-10, r0
        b       putchar
put_hex4_dec:
        addi    '0', r0
        b       putchar

;;; Print "0bXXXXXXXX"
;;; @param R1 value
;;; @clobber R0
put_bin8:
        pshr    r5
        pshr    r2
        pshr    r3
        mvii    '0', r0
        jsr     r5, putchar
        mvii    'b', r0
        jsr     r5, putchar
        movr    r1, r2
        swap    r2              ; bit 7 to bit 15
        mvii    8, r3
put_bin8_loop:
        mvii    '0', r0
        sllc    r2              ; C=msb
        bnc     put_bin8_bit
        incr    r0              ; '1'
put_bin8_bit:
        jsr     r5, putchar
        decr    r3
        bneq    put_bin8_loop
        pulr    r3
        pulr    r2
        pulr    r7

;;; Put a space (putspace), CR LF (newline) or char R0 (putchar)
putspace:
        mvii    ' ', r0
        b       putchar
newline:
        pshr    r5
        mvii    X'0D', r0
        jsr     r5, putchar
        mvii    X'0A', r0
        pulr    r5
putchar:
        pshr    r5
        pshr    r3
        mvii    tx_queue, r3
putchar_retry:
        jsrd    r5, queue_add
        eis
        bnc     putchar_retry   ; branch if queue is full
        mvii    RX_EN_TX_EN, r3
        mvo     r3, USARTC      ; Enable Rx/Tx
        pulr    r3
        pulr    r7

        include "queue.inc"

;;; The interrupt sequence leaves interrupts enabled, and #INTRM stays
;;; low until the data register is read, so USART_VEC disables them.
isr_usart:
        pshr    r0
        gswd    r0
        pshr    r0
        pshr    r3
        pshr    r5
        mvi     USARTS, r0
        andi    ST_RxRDY_bm, r0
        beq     isr_usart_tx
        mvi     USARTD, r0      ; Receive character
        mvii    rx_queue, r3
        jsr     r5, queue_add   ; Add to Rx queue
isr_usart_tx:
        mvi     USARTS, r0
        andi    ST_TxRDY_bm, r0
        beq     isr_usart_exit
        mvii    tx_queue, r3
        jsr     r5, queue_remove        ; Remove from Tx queue
        bnc     isr_usart_empty
        mvo     r0, USARTD      ; Transmit character
isr_usart_exit:
        pulr    r5
        pulr    r3
        pulr    r0
        rswd    r0
        pulr    r0
        eis
        pulr    r7              ; Return from interrupt
isr_usart_empty:
        mvii    RX_EN_TX_DIS, r0
        mvo     r0, USARTC      ; Disable Tx
        b       isr_usart_exit
