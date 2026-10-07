;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z380.inc"
        include "usart.inc"

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size

        org     1000H
stack:          equ     $

vec_base:       equ     $
        org     vec_base+12H
vec_rx: dw      isr_intr_rx
        org     vec_base+8AH
vec_tx: dw      isr_intr_tx

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
        xor     A               ; clear A
        outa    (USARTC), A
        outa    (USARTC), A
        outa    (USARTC), A          ; safest way to sync mode
        ld      A, CMD_IR_bm
        outa    (USARTC), A          ; reset
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

        ;; ld      A, HIGH vec_base
        ;; ld      I, A
        ;; ld      A, LOW vec_rx
        ;; outa    (USARTRV), A    ; set RxRDY interrupt vec_rx
        ;; ld      A, LOW vec_tx
        ;; outa    (USARTTV), A    ; set TxRDY interrupt vec_tx
        ;; im      2

        ei

loop:
        call    mandelbrot
        call    newline
        jr      loop

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
        align   2
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
        align   2
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
putchar_exit:
        pop     AF
        ret

;;; Put space
;;; @clobber A
        align   2
putspace:
        ld      A, ' '
        jr      putchar

        include "mandelbrot.inc"
        include "arith.inc"
        include "queue.inc"

        align   2
isr_intr_rx:
        ex      AF, AF'
        exall                   ; the ISR's own BC, DE, HL, IX and IY
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

        align   2
isr_intr_tx:
        ex      AF, AF'
        exall                   ; the ISR's own BC, DE, HL, IX and IY
        ina     A, (USARTS)
        bit     ST_TxRDY_bp, A
        jr      Z, isr_intr_tx_exit
        ld      IX, tx_queue
        call    queue_remove
        jr      NC,isr_intr_send_empty
        outa    (USARTD), A     ; send character
isr_intr_tx_exit:
        exall
        ex      AF, AF'
        ei
        reti
isr_intr_send_empty:
        ld      A, RX_EN_TX_DIS
        outa    (USARTC), A          ; disable Tx
        exall
        ex      AF, AF'
        ei
        reti

        end
