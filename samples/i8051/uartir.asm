;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8051
        include "i8051.inc"

        org     ORG_RESET
        ljmp    init
        org     ORG_IE0
        ljmp    init
        org     ORG_TF0
        ljmp    init
        org     ORG_IE1
        ljmp    init
        org     ORG_TF1
        ljmp    init
        org     ORG_RITI
        ljmp    isr_intr

;;; External data memory
        org     2000H
rx_buffer_size: equ     128
rx_buffer:      ds      rx_buffer_size

;;; Internal data memory
        org     BASE_MEMORY
rx_queue:       ds      queue_work_size
stack:          equ     $

init:
        mov     SP, #stack-1
        mov     R0, #rx_queue
        mov     R1, #rx_buffer_size
        mov     DPTR, #rx_buffer
        acall   queue_init
init_uart:
        mov     SCON, #(1 SHL SM1) ; 8-bit UART mode
        setb    SCON.REN        ; Enable reception
init_timer:
        orl     PCON, #SMOD     ; Double baudrate
        orl     TMOD, #T8MODE SHL T1MODE_gp
;;; baudrate = K*fosc/(32*12*(256-TH1)
;;; TH1=256 - (K*fosc/(384*baudrate))
;;; fosc=12MHz, K=2(SMOD=1) baudrate=4,800bps, TH1=243(256-13)
        mov     TL1, #243
        mov     TH1, #243
        setb    TCON.TR1        ; Run Timer 1
        setb    IE.ES           ; enable UART interrupt
        setb    IE.EA           ; enable interrupt

receive_loop:
        mov     R0, #rx_queue
        clr     IE.EA           ; disable interrupt
        acall   queue_remove
        setb    IE.EA           ; enable interrupt
        jnc     receive_loop
        jz      halt_to_system
        mov     R7, A           ; save character
transmit_loop:
        mov     SBUF, A
        jnb     SCON.TI, $
        clr     SCON.TI
        cjne    A, #0DH, receive_loop
        mov     A, #0AH
        sjmp    transmit_loop
halt_to_system:
        db      0A5H            ; halt to system

        include "queue.inc"

isr_intr:
        jbc     SCON.RI, isr_intr_rx
        reti
isr_intr_rx:
        push    PSW
        clr     PSW.RS1
        setb    PSW.RS0         ; select BANK 1
        mov     R1, DPL
        mov     R2, DPH         ; save DPTR
        mov     R3, A           ; save A
        mov     A, SBUF
        mov     R0, #rx_queue
        acall   queue_add
        mov     A, R3           ; restore A
        mov     DPH, R2
        mov     DPL, R1         ; restore DPTR
        pop     PSW
        reti
