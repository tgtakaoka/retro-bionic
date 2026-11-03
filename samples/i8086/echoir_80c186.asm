;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; On the BionicN80C186/N80C187 boards: INT0 in cascade mode, and an
;;; End Of Interrupt after each handler.
        cpu     80186
        include "usart.inc"

rx_queue:       equ     2000H
rx_queue_size:  equ     128

        org     VEC_RX*4
        dw      isr_intr_eoi, 0

        org     1000H
stack:  equ     $

init:
        xor     ax, ax
        mov     ds, ax
        mov     ss, ax
        mov     sp, stack
        mov     bx, rx_queue
        mov     cl, rx_queue_size
        call    queue_init
        mov     ah, RX_EN_TX_EN
        call    init_usart
        mov     al, VEC_RX
        out     USARTRV, al     ; enable RxRDY interrupt
        xor     al, al
        out     USARTTV, al     ; disable TxRDY interrupt
        call    init_icu
        sti

        include "echoir.inc"

        align   2
isr_intr_eoi:
        pushf
        callf   0:isr_intr
        jmp     isr_eoi

        include "icu_80c186.inc"

        org     0FFFF0H
        jmpf    0:init

        end
