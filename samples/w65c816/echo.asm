;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     65816
        .include "w65c816.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       $DF00
        .include "mc6850.inc"

        *=      $1000
stack   =       *-1
initialize:
        clc
        xce                     ; native mode
        rep     #P_X            ; 16-bit index
        longi   on
        ldx     #stack
        txs
        cld                     ; clear decimal flag
        lda     #CDS_RESET_gc   ; Master reset
        sta     ACIA_control
        lda     #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        sta     ACIA_control

loop:
        jsr     getchar
        ora     #0
        beq     halt_to_system
        jsr     putchar
        cmp     #$0D
        bne     loop
        lda     #$0A
        jsr     putchar
        bra     loop
halt_to_system:
        brk
        .byte     0             ; halt to system

getchar:
        php
        sep     #P_M            ; 8-bit memory
getchar_loop:
        lda     ACIA_status
        and     #RDRF_bm
        beq     getchar_loop
        lda     ACIA_data
        plp
        rts

putchar:
        php
        sep     #P_M            ; 8-bit memory
        pha
putchar_loop:
        lda     ACIA_status
        and     #TDRE_bm
        beq     putchar_loop
        pla
        sta     ACIA_data
        plp
        rts

        *=      VEC_RESET
        .word   initialize
