;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     1804A
        option  "smart-branch", "on"
        include "cdp1802.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     X'DF00'
        include "mc6850.inc"

        org     X'0100'
main:
        ldi     A.1(ACIA)
        phi     R8
        ldi     A.0(ACIA)
        plo     R8              ; R8=ACIA
        ldi     CDS_RESET_gc    ; Master reset
        str     R8              ; ACIA_control
        ldi     WSB_8N1_gc      ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        str     R8              ; ACIA_control

receive_loop:
        ldn     R8              ; ACIA_status
        ani     RDRF_bm
        bz      receive_loop
receive_data:
        inc     R8              ; R8=ACIA_data
        ldn     R8              ; d=char
        bz      halt_to_system
transmit:
        dec     R8              ; R8=ACIA_status
        plo     R7              ; R7.0=char
transmit_loop:
        ldn     R8              ; ACIA_status
        ani     TDRE_bm
        bz      transmit_loop
transmit_data:
        glo     R7
        inc     R8              ; R8=ACIA_data
        str     R8              ; ACIA_data
        dec     R8              ; R8=ACIA_status
        xri     X'0D'
        bnz     receive_loop
        ldi     X'0A'
        plo     R7              ; R7.0=char
        br      transmit_loop

        org     ORG_RESET
        dis                     ; IE=0
        dc      X'00'           ; X:P=0:0
        ldi     A.1(main)
        phi     R3
        ldi     A.0(main)
        plo     R3
        sep     R3               ; jump to main with R3 as PC
halt_to_system:
        idl
