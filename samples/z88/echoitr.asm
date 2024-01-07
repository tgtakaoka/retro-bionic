;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z88
        option  "reg-alias", "disable"
        option  "optimize-index", "enable"

        include "z88.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USART:          equ     %FF00
USARTD:         equ     0       ; Receive/Transmit data
USARTS:         equ     1       ; Status register
USARTC:         equ     1       ; Control register
USARTRI:        equ     2       ; Receive interrupt name (IRQ0~2)
USARTTI:        equ     3       ; Transmit interrupt name (IRQ0~2)
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc LOR MODE_LEN8_gc LOR MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm LOR CMD_TxEN_bm
RX_EN_TX_DIS:   equ     CMD_RTS_bm LOR CMD_DTR_bm LOR CMD_ER_bm LOR CMD_RxEN_bm

        org     %2000
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size

        org     %1000
stack:  equ     $

        org     VEC_IRQ0_P23
        dw      isr_intr_rx

        org     VEC_IRQ1_P21
        dw      isr_intr_tx

        org     ORG_RESET
        setrp   %C0
init_config:
        ld      EMT, #EMT_STACK_DM ; stack is on external data memory
        ld      P0M, #P0M_ADRH     ; Port 0 is address bus high
        ld      PM, #PM_P1BUS | PM_DM ; Port 1 is data bus and address bus low
        ldw     SPH, #stack
        ldw     rr2, #rx_queue
        ld      r1, #rx_queue_size
        call    queue_init
        ldw     rr2, #tx_queue
        ld      r1, #tx_queue_size
        call    queue_init

init_usart:
        ldw     rr12, #USART
        clr     r0
        lde     USARTC(rr12), r0
        lde     USARTC(rr12), r0
        lde     USARTC(rr12), r0 ; safest way to sync mode
        ld      r0, #CMD_IR_bm
        lde     USARTC(rr12), r0 ; reset
        nop
        nop
        ld      r0, #ASYNC_MODE
        lde     USARTC(rr12), r0 ; async 1stop 8data x16
        nop
        nop
        ld      r0, #RX_EN_TX_DIS
        lde     USARTC(rr12), r0 ; RTS/DTR, error reset, Rx enable, Tx disable
        ld      r0, #INTR_IRQ0
        lde     USARTRI(rr12), r0 ; enable RxRDY interrupt using IRQ0
        ld      r0, #INTR_IRQ1
        lde     USARTTI(rr12), r0 ; enable TxRDY interrupt using IRQ1

        ld      P2BM, #P2M_INTR_gm SHL 2 ; P23=input, interrupt enabled
        ld      P2AM, #P2M_INTR_gm SHL 2 ; P21=input, interrupt enabled
        ld      IPR, #IPR_ABC ; (IRQ0 > IRQ1) > (IRQ2,3,4) > (IRQ5,6,7)
        ld      IMR, #IMR_IRQ0 LOR IMR_IRQ1 ; enable IRQ0, IRQ1
        ei

receive_loop:
        call    getchar
        jr      nc, receive_loop
        or      r0,r0
        jr      nz,echo_back
        wfi                     ; halt to system
echo_back:
        ld      r1, r0          ; save letter
        call    putchar         ; echo
        ld      r0, #' '        ; space
        call    putchar
        call    put_hex8        ; print in hex
        ld      r0, #' '        ; space
        call    putchar
        call    put_bin8        ; print in binary
        call    newline
        jr      receive_loop

;;; Put newline
;;; @clobber r0
newline:
        ld      r0, #%0D
        call    putchar
        ld      r0, #%0A
        jr      putchar

;;; Print uint8_t in hex
;;; @param r1 uint8_t value to be printed in hex.
;;; @clobber r0
put_hex8:
        ld      r0, #'0'
        call    putchar
        ld      r0, #'x'
        call    putchar
        ld      r0, r1
        swap    r0
        call    put_hex4
        ld      r0, r1
put_hex4:
        and     r0, #%F
        cp      r0, #10
        jr      c, put_hex8_dec ; A<10
        add     r0, #'A'-10
        jr      putchar
put_hex8_dec:
        add     r0, #'0'
        jr      putchar

;;; Print uint8_t in binary
;;; @param r1 uint8_t value to be printed in binary.
;;; @clobber r0
put_bin8:
        push    r4
        ld      r0, #'0'
        call    putchar
        ld      r0, #'b'
        call    putchar
        ld      r4, r1
        call    put_bin4
        rl      r4
        call    put_bin4
        pop     r4
        ret
put_bin4:
        call    put_bin2
        rl      r4
put_bin2:
        call    put_bin1
        rl      r4
put_bin1:
        ld      r0, #'0'
        or      r4, r4
        jp      pl, put_bin0    ; MSB=0
        ld      r0, #'1'        ; MSB=1
put_bin0:
        jr      putchar

;;; Get character
;;; @return r0
;;; @return FLAGS.C 0 if no character
getchar:
        push    r3
        push    r2
        ldw     rr2, #rx_queue
        di
        call    queue_remove
        ei
        pop     r2
        pop     r3
        ret

;;; Put character
;;; @param r0
putchar:
        push    r0
        push    r3
        push    r2
        ldw     rr2, #tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jr      nc, putchar_retry ; branch if queue is full
        ld      r0, #RX_EN_TX_EN
        lde     USARTC(rr12), r0 ; enable Tx
putchar_exit:
        pop     r2
        pop     r3
        pop     r0
        ret

        include "queue.inc"

        setrp   -1
isr_intr_rx:
        ld      P2AIP, #1 SHL 5 ; clear P23 IRQ0
        push    r0
        push    r3
        push    r2
        ldw     rr2, #USART
        lde     r0, USARTS(rr2)
        and     r0, #ST_RxRDY_bm
        jr      z, isr_intr_rx_exit
        lde     r0, USARTD(rr2)
        ldw     rr2, #rx_queue
        call    queue_add
isr_intr_rx_exit:
        pop     r2
        pop     r3
        pop     r0
        iret

isr_intr_tx:
        ld      P2AIP, #1 SHL 1 ; clear P21 IRQ1
        push    r0
        push    r3
        push    r2
        ldw     rr2, #USART
        lde     r0, USARTS(rr2)
        and     r0, #ST_TxRDY_bm
        jr      z, isr_intr_tx_exit
        ldw     rr2, #tx_queue
        call    queue_remove
        ldw     rr2, #USART
        jr      nc, isr_intr_send_empty
        lde     USARTD(rr2), r0
isr_intr_tx_exit:
        pop     r2
        pop     r3
        pop     r0
        iret
isr_intr_send_empty:
        ld      r0, #RX_EN_TX_DIS
        lde     USARTC(rr2), r0 ; disable Tx
        pop     r2
        pop     r3
        pop     r0
        iret

        end
