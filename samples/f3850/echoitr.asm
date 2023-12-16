;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     f3850

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     0F0H
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

        org     1000H
stack:

        org     2000H
rx_queue_size:  equ     128
rx_queue:       rs      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       rs      tx_queue_size

        org     0
        jmp     init

rx_vec:
        jmp     isr_intr_rx
tx_vec:
        jmp     isr_intr_tx

init:
        pi      init_stack
        dci     rx_queue
        lr      Q, DC
        li      rx_queue_size
        lr      0, A
        pi      call
        da      queue_init
        dci     tx_queue
        lr      Q, DC
        li      tx_queue_size
        lr      0, A
        pi      call
        da      queue_init
init_usart:
        lis     0
        out     USARTC
        out     USARTC
        out     USARTC          ; safest way to sync mode
;;; reset
        li      CMD_IR_bm
        out     USARTC
        nop
        nop
        li      ASYNC_MODE
        out     USARTC
        nop
        nop
        li      RX_EN_TX_DIS
        out     USARTC
        li      rx_vec
        out     USARTRV
        li      tx_vec
        out     USARTTV

receive_loop:
        pi      call
        da      getchar
        bnc     receive_loop
        lr      A, 0
        oi      0
        bz      halt_to_system
echo_back:
        lr      1, A            ; save character
        pi      call
        da      putchar         ; echo
        li      C' '            ; space
        lr      0, A
        pi      call
        da      putchar
        lr      A, 1
        lr      0, A
        pi      call
        da      put_hex8        ; print in hex
        li      C' '            ; space
        lr      0, A
        pi      call
        da      putchar
        lr      A, 1
        lr      0, A
        pi      call
        da      put_bin8        ; print in binary
        li      H'0D'
        lr      0, A
        pi      call
        da      putchar
        li      H'0A'
        lr      0, A
        pi      call
        da      putchar
        br      receive_loop
halt_to_system:
        dc      H'2F'           ; unknown instruction

;;; Print uint8_t in hex
;;; @param 1 uint8_t value to be printed in hex.
;;; @clobber 0 A
put_hex8:
        li      C'0'
        lr      0, A
        pi      call
        da      putchar
        li      C'x'
        lr      0, A
        pi      call
        da      putchar
        lr      A, 1
        sr      4
        lr      0, A
        pi      call
        da      put_hex4
        lr      A, 1
        lr      0, A
put_hex4:
        lr      A, 0
        ni      H'0F'
        ci      9               ; 9-A
        bp      put_hex8_dec    ; A<10
        ai      C'A'-10
        lr      0, A
        jmp     putchar
put_hex8_dec:
        ai      C'0'
        lr      0, A
        jmp     putchar

;;; Print uint8_t in binary
;;; @param 1 uint8_t value to be printed in binary.
;;; @clobber 0 A
put_bin8:
        li      C'0'
        lr      0, A
        pi      call
        da      putchar
        li      C'b'
        lr      0, A
        pi      call
        da      putchar
        lr      A, 1
        lr      0, A
        pi      call
        da      put_bin4
        lr      A, 1
        sl      4
        lr      0, A
;;; Pinrt uint4_t in binary
;;; @param 0 4-bit value in upper nibble
;;; @clobber 0 A
put_bin4:
        pi      push0
        pi      call
        da      put_bin2
        pi      pop0
        lr      A, 0
        sl      1
        sl      1
        lr      0, A
;;; Print uint2_t in binary
;;; @param 0 value in the most upper 2-bit
;;; @clobber 0 A
put_bin2:
        pi      push0
        pi      call
        da      put_bin1
        pi      pop0
        lr      A, 0
        sl      1
        lr      0, A
;;; Print MSB
;;; @param 0 value at MSB
;;; @clobber 0 A
put_bin1:
        lr      A, 0
        oi      0
        li      C'0'            ; '0' for bit value 0
        bp      put_bin0        ; MSB=0
        inc                     ; make it '1'
put_bin0
        lr      0, A
        jmp     putchar

;;; Get character
;;; @return 0
;;; @return CC.C 0 if no character
getchar:
        ei
        nop
        di
        dci     rx_queue
        lr      Q, DC
        jmp     queue_remove

;;; Put character
;;; @param 0
;;; @clobber 0
putchar:
        ei
        nop
        di
        dci     tx_queue
        lr      Q, DC
        pi      call
        da      queue_add
        bnc     putchar         ; branch if queue is full
        li      RX_EN_TX_EN     ; enable Tx
        out     USARTC
        jmp     return

isr_intr_rx:
        lr      K, P
        pi      pushK
        pi      push0
        in      USARTS
        ni      ST_RxRDY_bm
        bz      isr_intr_end
        in      USARTD
        lr      0, A
        dci     rx_queue
        lr      Q, DC
        pi      call
        da      queue_add
isr_intr_end:
        pi      pop0
        jmp     return

isr_intr_tx:
        lr      K, P
        pi      pushK
        pi      push0
        dci     tx_queue
        lr      Q, DC
        pi      call
        da      queue_remove
        bnc     isr_intr_send_empty
        lr      A, 0
        out     USARTD          ; send character
        pi      pop0
        jmp     return
isr_intr_send_empty:
        li      RX_EN_TX_DIS    ; disable Tx
        out     USARTC
        pi      pop0
        jmp     return

        include "stack.inc"
        include "queue.inc"
