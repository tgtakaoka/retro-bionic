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
        ljmp    init

init:
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

receive_loop:
        jbc     SCON.RI, receive_data
        sjmp    receive_loop
receive_data:
        mov     A, SBUF
        jz      halt_to_system
transmit_loop:
        mov     SBUF, A
        jnb     SCON.TI, $
        clr     SCON.TI
        cjne    A, #0DH, receive_loop
        mov     A, #0AH
        sjmp    transmit_loop
halt_to_system:
        db      0A5H            ; halt to system
