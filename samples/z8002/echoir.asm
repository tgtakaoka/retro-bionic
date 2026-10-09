;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "z8002.inc"
        include "usart.inc"

rx_queue:       equ     2000H
rx_queue_size:  equ     128

stack:  equ     1000H

        org     ORG_RESET
        word    0
        word    FCW_SN          ; system mode, interrupts disabled
        word    init

        org     PSA_NVI
        word    FCW_SN          ; the handler runs with interrupts disabled
        word    isr_nvi

        org     0100H
init:
        ld      r15, #stack
        clr     r0
        ldctl   psap, r0        ; the Program Status Area at 0000H
        ld      r1, #rx_queue
        ldb     rl0, #rx_queue_size
        call    queue_init
init_usart:
        clrb    rl0
        outb    USARTC, rl0
        outb    USARTC, rl0
        outb    USARTC, rl0     ; safest way to sync mode
        ldb     rl0, #CMD_IR_bm
        outb    USARTC, rl0     ; reset
        nop
        nop
        ldb     rl0, #ASYNC_MODE
        outb    USARTC, rl0
        nop
        nop
        ldb     rl0, #RX_EN_TX_EN
        outb    USARTC, rl0
        ldb     rl0, #1
        outb    USARTRV, rl0    ; enable RxRDY interrupt, identifier 1
        clrb    rl0
        outb    USARTTV, rl0    ; disable TxRDY interrupt
        ei      nvi

receive_loop:
        ld      r1, #rx_queue
        di      nvi
        call    queue_remove
        ei      nvi
        jr      nc, receive_loop
        ldb     rl3, rl0        ; save character
        testb   rl3
        jr      z, halt_to_system
transmit_loop:
        inb     rl0, USARTS
        bitb    rl0, #ST_TxRDY_bp
        jr      z, transmit_loop
transmit_data:
        outb    USARTD, rl3
        cpb     rl3, #0DH
        jr      ne, receive_loop
        ldb     rl3, #0AH
        jr      transmit_loop
halt_to_system:
        sc      #SC_EXIT

        include "queue.inc"

isr_nvi:
        push    @r15, r0
        push    @r15, r1
        inb     rl0, USARTS
        bitb    rl0, #ST_RxRDY_bp
        jr      z, isr_nvi_exit
        inb     rl0, USARTD
        ld      r1, #rx_queue
        call    queue_add
isr_nvi_exit:
        pop     r1, @r15
        pop     r0, @r15
        jp      isr_return

;;; Return from an interrupt handler.
isr_return:
        iret
