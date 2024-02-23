        cpu     6309
        include "mc6809.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "mc6850.inc"

        org     $1000
stack:  equ     *

        org     VEC_RESET
        fdb     initialize

        org     $0100
initialize:
        ldmd    #1              ; 6309 native mode
        lds     #stack
        lda     #CDS_RESET_gc   ; Master reset
        sta     ACIA_control
        lda     #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        sta     ACIA_control

loop:   bsr     getchar
        tsta
        beq     halt_to_system
echo:   bsr     putchar
        cmpa    #$0D
        bne     loop
        lda     #$0A
        bra     echo
halt_to_system:
        ldx     #step_swi
        stx     VEC_SWI
        swi
step_swi:
        ldx     #VEC_SWI
        stx     VEC_SWI
        swi                 ; halt to system

getchar:
        lda     ACIA_status
        bita    #RDRF_bm
        beq     getchar
        lda     ACIA_data
        rts

putchar:
        ldb     ACIA_status
        bitb    #TDRE_bm
        beq     putchar
        sta     ACIA_data
        rts
