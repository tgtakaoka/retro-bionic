;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z180.inc"
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
        ld      HL, rx_queue
        ld      B, rx_queue_size
        call    queue_init
        ld      HL, tx_queue
        ld      B, tx_queue_size
        call    queue_init
init_usart:
        ld      BC, USARTC
        xor     A               ; clear A
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

        ;; ld      A, HIGH vec_base
        ;; ld      I, A
        ;; ld      A, LOW vec_rx
        ;; ld      BC, USARTRV
        ;; out     (C), A       ; set RxRDY interrupt vec_rx
        ;; ld      A, LOW vec_tx
        ;; ld      BC, USARTTV
        ;; out     (C), A       ; set TxRDY interrupt vec_tx
        ;; im      2

        ei

loop:
        call    mandelbrot
        call    newline
        jr      loop

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
        push    BC
        ld      a, RX_EN_TX_EN  ; enable Tx
        ld      BC, USARTC
        out     (C), A          ; (USARTC)
        pop     BC
putchar_exit:
        pop     AF
        ret

;;; Put newline
;;; @clobber A
putspace:
        ld      A, ' '
        jr      putchar

        include "../z80/mandelbrot.inc"
        include "arith.inc"
        include "../z80/queue.inc"

isr_intr_rx:
        push    AF
        push    BC
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
        jr      NC,isr_intr_send_empty
        ld      BC, USARTD
        out     (C), A          ; send character
isr_intr_tx_exit:
        pop     BC
        pop     AF
        ei
        reti
isr_intr_send_empty:
        ld      a, RX_EN_TX_DIS
        ld      BC, USARTC
        out     (C), A          ; disable Tx
        pop     BC
        pop     AF
        ei
        reti

        end
