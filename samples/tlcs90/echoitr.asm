;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tlcs90
        include "tmp90c802.inc"

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

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jp      init

        org     ORG_SWI
        halt                    ; halt to system

        org     ORG_INT0
        jp      isr_intr_rx

        org     ORG_INT1
        jp      isr_intr_tx

        org     0100H
init:
        ld      sp, stack
        ld      ix, rx_queue
        ld      b, rx_queue_size
        call     queue_init
        ld      ix, tx_queue
        ld      b, tx_queue_size
        call     queue_init
init_usart:
        ld      (USARTC), 0
        ld      (USARTC), 0
        ld      (USARTC), 0     ; safest way to sync mode
        ld      (USARTC), CMD_IR_bm
        nop
        nop
        ld      (USARTC), ASYNC_MODE
        nop
        nop
        ld      (USARTC), RX_EN_TX_DIS ; disable Tx
        ld      (USARTRV), ORG_INT0    ; enable Rx interrupt using INT0
        ld      (USARTTV), ORG_INT1    ; enable Tx interrupt using INT1
        set     INTEH_IE0_bp, (INTEH)  ; Enable INT0
        set     INTEL_IE1_bp, (INTEL)  ; Enable INT1
        ei

receive_loop:
        call    getchar
        jr      nc, receive_loop
        or      a, a
        jr      nz, echo_back
        swi
echo_back:
        ld      b, a
        call    putchar         ; echo
        ld      a, ' '          ; space
        call    putchar
        call    put_hex8        ; print in hex
        ld      a, ' '          ; space
        call    putchar
        call    put_bin8        ; print in binary
        call    newline
        jr      receive_loop

;;; Put newline
;;; @clobber A
newline:
        ld      a, 0DH
        call    putchar
        ld      a, 0AH
        jp      putchar

;;; Print uint8_t in hex
;;; @param B uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        ld      a, '0'
        call    putchar
        ld      a, 'x'
        call    putchar
        ld      a, b
        srla
        srla
        srla
        srla
        call    put_hex4
        ld      a, b
put_hex4:
        and     a, 0FH
        cp      a, 10
        jr      c, put_hex8_dec    ; A<10
        add     a, 'A'-10
        jp      putchar
put_hex8_dec:
        add     a, '0'
        jp      putchar

;;; Print uint8_t in binary
;;; @param B uint8_t value to be printed in binary.
;;; @clobber A
put_bin8:
        push    bc
        ld      a, '0'
        call    putchar
        ld      a, 'b'
        call    putchar
        call    put_bin4
        call    put_bin4
        pop     bc
        ret
put_bin4:
        call    put_bin2
put_bin2:
        call    put_bin1
put_bin1:
        sll     b               ; C=MSB
        ld      a, '0'
        jr      nc, putchar     ; MSB=0
        inc     a
        jr      putchar

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
getchar:
        push    ix
        ld      ix, rx_queue
        di
        call    queue_remove
        ei
        pop     ix
        ret

;;; Put character
;;; @param A
putchar:
        push    af
        push    ix
        ld      ix, tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jr      nc, putchar_retry ; branch if queue is full
        ld      (USARTC), RX_EN_TX_EN ; enable Tx
        pop     ix
        pop     af
        ret

        include "queue.inc"

isr_intr:
        bit     ST_RxRDY_bp, (USARTS)
        jr      nz, isr_intr_rx
        bit     ST_TxRDY_bp, (USARTS)
        jr      nz, isr_intr_tx
        reti

isr_intr_rx:
        push    ix
        ld      a, (USARTD)     ; receive character
        ld      ix, rx_queue
        call    queue_add
        pop     ix
        reti

isr_intr_tx:
        push     ix
        ld      ix, tx_queue
        call    queue_remove
        pop     ix
        jr      nc, isr_intr_send_empty
        ld      (USARTD), a     ; send character
        reti
isr_intr_send_empty:
        ld      (USARTC), RX_EN_TX_DIS ; disable Tx
        reti
