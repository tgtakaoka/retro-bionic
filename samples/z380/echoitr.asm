;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z380.inc"
        include "usart.inc"

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jp      init

        org     ORG_RST28
        jp      isr_intr_rx

        org     ORG_RST30
        jp      isr_intr_tx

        org     0100H
init:
        ld      SP, stack
        ld      IX, rx_queue
        ld      B, rx_queue_size
        call    queue_init
        ld      IX, tx_queue
        ld      B, tx_queue_size
        call    queue_init
init_usart:
        xor     a               ; clear A
        outa    (USARTC), A
        outa    (USARTC), A
        outa    (USARTC), A     ; safest way to sync mode
        ld      A, CMD_IR_bm
        outa    (USARTC), A     ; reset
        nop
        nop
        ld      A, ASYNC_MODE
        outa    (USARTC), A
        nop
        nop
        ld      A, RX_EN_TX_DIS
        outa    (USARTC), A
        db      3EH             ; LD A, n
        rst     28H
        outa    (USARTRV), A    ; set RxRDY interrupt vector RST 28H
        db      3EH             ; LD A, n
        rst     30H
        outa    (USARTTV), A    ; set TxRDY interrupt vector RST 30H

        im      0
        ei

receive_loop:
        call    getchar
        jr      NC, receive_loop
        or      A
        jr      Z, halt_to_system
echo_back:
        ld      B, A
        call    putchar         ; echo
        ld      A, ' '          ; space
        call    putchar
        call    put_hex8        ; print in hex
        ld      A, ' '          ; space
        call    putchar
        call    put_bin8        ; print in binary
        call    newline
        jr      receive_loop
halt_to_system:
        ld      A, 0FFH
        ld      (ORG_RST38), A
        rst     38h

;;; Print uint8_t in hex
;;; @param B uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        ld      A, '0'
        call    putchar
        ld      A, 'x'
        call    putchar
        ld      A, b
        srl     A
        srl     A
        srl     A
        srl     A
        call    put_hex4
        ld      A, B
put_hex4:
        and     0FH
        add     A, 90H
        daa
        adc     A, 40H
        daa
        jr      putchar

;;; Print uint8_t in binary
;;; @param B uint8_t value to be printed in binary.
;;; @clobber A
put_bin8:
        push    BC
        ld      A, '0'
        call    putchar
        ld      A, 'b'
        call    putchar
        ld      A, B
        call    put_bin4
        call    put_bin4
        pop     BC
        ret
put_bin4:
        call    put_bin2
put_bin2:
        call    put_bin1
put_bin1:
        ld      A, B
        sla     A               ; F.C=MSB
        ld      B, A
        ld      A, '0'
        jr      NC, putchar     ; F.0=1
        inc     A               ; F.C=1
        jr      putchar

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
getchar:
        push    IX
        ld      IX, rx_queue
        di
        call    queue_remove
        ei
        pop     IX
        ret

;;; Put newline
;;; @clobber A
newline:
        ld      A, 0DH
        call    putchar
        ld      A, 0AH

;;; Put character
;;; @param A
putchar:
        push    AF
        push    IX
        ld      IX, tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jr      NC, putchar_retry ; branch if queue is full
        pop     IX
        ld      A, RX_EN_TX_EN  ; enable Tx
        outa    (USARTC), A
        pop     AF
        ret

        include "queue.inc"

isr_intr_rx:
        ex      AF, AF'
        exall                   ; the ISR's own BC, DE, HL, IX and IY
isr_intr_receive:
        ina     A, (USARTS)
        bit     ST_RxRDY_bp, A
        jr      Z, isr_intr_rx_exit
        ina     A, (USARTD)     ; receive character
        ld      IX, rx_queue
        call    queue_add
isr_intr_rx_exit:
        exall
        ex      AF, AF'
        ei
        reti

isr_intr_tx:
        ex      AF, AF'
        exall                   ; the ISR's own BC, DE, HL, IX and IY
        ina     A, (USARTS)
        bit     ST_TxRDY_bp, A
        jr      Z, isr_intr_tx_exit
        ld      IX, tx_queue
        call    queue_remove
        jr      NC, isr_intr_send_empty
        outa    (USARTD), A     ; send character
isr_intr_tx_exit:
        exall
        ex      AF, AF'
        ei
        reti
isr_intr_send_empty:
        ld      A, RX_EN_TX_DIS
        outa    (USARTC), A     ; disable Tx
        exall
        ex      AF, AF'
        ei
        reti

        end
