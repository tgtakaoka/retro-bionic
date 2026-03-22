;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z180.inc"
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
        ld      HL, rx_queue
        ld      B, rx_queue_size
        call    queue_init
        ld      HL, tx_queue
        ld      B, tx_queue_size
        call    queue_init
init_usart:
        xor     a               ; clear A
        ld      BC, USARTC
        out     (C), A          ; (USARTC)
        out     (C), A          ; (USARTC)
        out     (C), A          ; safest way to sync mode
        ld      A, CMD_IR_bm
        out     (C), A          ; reset
        nop
        nop
        ld      A, ASYNC_MODE
        out     (C), A
        nop
        nop
        ld      A, RX_EN_TX_DIS
        out     (C), A
        db      3EH             ; LD A, n
        rst     28H
        ld      BC, USARTRV
        out     (C), A          ; set RxRDY interrupt vector RST 28H
        db      3EH             ; LD A, n
        rst     30H
        ld      BC, USARTTV
        out     (C), A          ; set TxRDY interrupt vector RST 30H

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
        ld      HL, ORG_RST38
        ld      (HL), 0FFH
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
        push    HL
        ld      HL, rx_queue
        di
        call    queue_remove
        ei
        pop     HL
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
        push    HL
        ld      HL, tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jr      NC, putchar_retry ; branch if queue is full
        pop     HL
        ld      A, RX_EN_TX_EN  ; enable Tx
        push    BC
        ld      BC, USARTC
        out     (C), A
        pop     BC
        pop     AF
        ret

        include "../z80/queue.inc"

isr_intr_rx:
        push    AF
        push    BC
isr_intr_receive:
        ld      BC, USARTS
        in      A, (C)          ; (USARTS)
        bit     ST_RxRDY_bp, A
        jr      Z, isr_intr_rx_exit
        ld      BC, USARTD
        in      A, (C)          ; receive character
        push    HL
        ld      HL, rx_queue
        call    queue_add
        pop     HL
isr_intr_rx_exit:
        pop     BC
        pop     AF
        ei
        reti

isr_intr_tx:
        push    AF
        push    BC
        ld      BC, USARTS
        in      A, (C)          ; (USARTS)
        bit     ST_TxRDY_bp, A
        jr      Z, isr_intr_tx_exit
        push    HL
        ld      HL, tx_queue
        call    queue_remove
        pop     HL
        jr      NC, isr_intr_send_empty
        ld      BC, USARTD
        out     (C), A          ; send character
isr_intr_tx_exit:
        pop     BC
        pop     AF
        ei
        reti
isr_intr_send_empty:
        ld      A, RX_EN_TX_DIS
        ld      bc, USARTC
        out     (C), A          ; disable Tx
        pop     BC
        pop     AF
        ei
        reti

        end
