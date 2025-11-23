;;; -*- mode: asm; mode: flyspell-prog; -*-
        .include "mos6502.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       $DF00
        .include "mc6850.inc"


        *=      VEC_RESET
        .word   initialize

        *=      $1000
stack   =       $01ff
initialize:
        ldx     #stack & 0xFF
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
        jmp     loop
halt_to_system:
        brk
        .byte     0             ; halt to system

getchar:
        lda     ACIA_status
        and     #RDRF_bm
        beq     getchar
        lda     ACIA_data
        rts

putchar:
        pha
putchar_loop:
        lda     ACIA_status
        and     #TDRE_bm
        beq     putchar_loop
        pla
        sta     ACIA_data
        rts
