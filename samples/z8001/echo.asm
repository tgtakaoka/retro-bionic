;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z8001
        include "z8001.inc"
        include "../z8002/usart.inc"

        org     ORG_RESET
        word    0
        word    FCW_SEG|FCW_SN  ; segmented system mode, interrupts disabled
        word    CODE_SEG, init-CODE

        org     CODE+0100H
init:
        ldl     rr14, #STACK
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

receive_loop:
        inb     rl0, USARTS
        bitb    rl0, #ST_RxRDY_bp
        jr      z, receive_loop
receive_data:
        inb     rl3, USARTD
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
