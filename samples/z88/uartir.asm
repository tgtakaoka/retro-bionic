;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z88
        option  "reg-alias", "disable"
        option  "optimize-index", "enable"

        include "z88.inc"

        org     %2000
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size

        org     %1000
stack:  equ     $

        org     VEC_UART_RECV   ; IRQ6
        dw      isr_intr

        org     ORG_RESET
        setrp   %C0
init_config:
        ld      EMT, #EMT_STACK_DM ; stack is on external data memory
        ld      P0M, #P0M_ADRH     ; Port 0 is address bus high
        ld      PM, #PM_P1BUS LOR PM_DM ; Port 1 is data bus and address bus low
        ldw     SPH, #stack

        ldw     rr2, #rx_queue
        ld      r1, #rx_queue_size
        call    queue_init

;;; XTAL=14.7546MHz
;;; clock divider N=32, baud-rate generator UBG=11
;;; bit rate = (14,754,600 / 4) / (2 x (UBG+1) x N) = 9600 bps
init_uart:
        ld      P2AM, #P2M_OUTPP_gm SHL 6   ; P31/TXD=output
        or      PORT3, #1                   ; TXD=high
        or      FLAGS, #F_BANK              ; select bank1
        ld      UMA, #UMA_CR32 LOR UMA_BCP8 ; clock rate x32, 8 bit char
        ldw     UBG0, #11                   ; UBG=11
        ld      r0, #UMB_BRGSRC LOR UMB_BRGENB ; enable baud-rate generator, select XTAL/4
        or      r0, #UMB_RCIS LOR UMB_TCIS ; use baud-rate generator for Rx and Tx
        ld      UMB, r0
        and     FLAGS, #LNOT F_BANK            ; select bank0
        ld      URC, #URC_RENB                 ; enable receiver
        ld      UTC, #UTC_TENB LOR UTC_TXDTSEL ; enable transmit and TxD
        ld      UIE, #UIE_RCAIE                ; enable receive interrupt

init_irq:
        ld      IPR, #IPR_CAB LOR IPR_C675
        ld      IMR, #IMR_IRQ6 ; enable IRQ6
        ei                     ; clear IRQ and enable interrupt system

receive_loop:
        di                      ; Disable INTR
        call    queue_remove
        ei                      ; Enable INTR
        jr      nc, receive_loop
        or      r0,r0
        jr      nz,transmit_loop
        wfi                     ; halt to system
transmit_loop:
        tm      UTC, #UTC_TBE   ; check transmit buffer empty
        jr      z, transmit_loop
transmit_data:
        ld      UIO, r0
        cp      r0, #%0D
        jr      nz, receive_loop
        ld      r0, #%0A
        jr      transmit_loop

        include "queue.inc"

        setrp   -1
isr_intr:
        push    r0
        tm      URC, #URC_RCA
        jr      z, isr_intr_end
        ld      r0, UIO
        push    r3
        push    r2
        ldw     rr2, #rx_queue
        call    queue_add
        pop     r2
        pop     r3
        pop     r0
isr_intr_end:
        iret
