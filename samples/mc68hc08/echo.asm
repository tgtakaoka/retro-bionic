        include "mc68hc08az0.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FFE0
        include "mc6850.inc"

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $1000
stack:  equ     *
initialize:
        ldhx    #stack
        txs
        lda     #CDS_RESET_gc   ; Master reset
        sta     ACIA_control
        lda     #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        sta     ACIA_control

loop:   bsr     getchar
        tsta
        beq     halt_to_system
echo:   bsr     putchar
        cmp     #$0D            ; Carriage Return
        bne     loop
        lda     #$0A            ; Newline
        bra     echo
halt_to_system:
        swi                     ; halt to system

getchar:
        lda     COP_RESET
        sta     COP_RESET
        lda     ACIA_status
        bit     #RDRF_bm
        beq     getchar
        lda     ACIA_data
        rts

putchar:
        psha
transmit_loop:
        lda     COP_RESET
        sta     COP_RESET
        lda     ACIA_status
        bit     #TDRE_bm
        beq     transmit_loop
        pula
        sta     ACIA_data
        rts
