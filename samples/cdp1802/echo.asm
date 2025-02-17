;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     1802
        option  "smart-branch", "on"
        include "cdp1802.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     4
        include "mc6850.inc"

        org     X'0100'
main:
        ldi     A.1(ACIA_config)
        phi     R8
        ldi     A.0(ACIA_config)
        plo     R8              ; R8=ACIA
        sex     R8              ; R8 for inp/out
        out     ACIA_control    ; Master reset
        out     ACIA_control    ; Mode set
        br      receive

ACIA_config:
        dc      CDS_RESET_gc    ; Master reset
        dc      WSB_8N1_gc      ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
received_char:
        dc      0
receive:
        ldi     A.1(received_char)
        phi     R8
        ldi     A.0(received_char)
        plo     R8
        sex     R8              ; R8 for inp/out
receive_loop:
        inp     ACIA_status
        ani     RDRF_bm
        bz      receive_loop
receive_data:
        inp      ACIA_data
        bz      halt_to_system
transmit:
        plo     R7              ; R7.0=char
transmit_loop:
        inp     ACIA_status     ; ACIA_status
        ani     TDRE_bm
        bz      transmit_loop
transmit_data:
        glo     R7
        str     R8              ; set received_char
        out     ACIA_data
        dec     R8              ; R8=received_char
        xri     X'0D'
        bnz     receive
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
