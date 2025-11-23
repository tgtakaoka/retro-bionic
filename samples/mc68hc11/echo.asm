        include "mc68hc11.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "../mc6800/mc6850.inc"

        org     $1000
stack:  equ     *-1             ; MC6801's SP is post-decrement/pre-increment

        org     VEC_RESET
        fdb     initialize

        org     $0100
initialize:
        lds     #stack
        ldaa    #CDS_RESET_gc   ; Master reset
        staa    ACIA_control
        ldaa    #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        staa    ACIA_control

loop:   bsr     getchar
        tsta
        beq     halt_to_system
echo:   bsr     putchar
        cmpa    #$0D
        bne     loop
        ldaa    #$0A
        bra     echo
halt_to_system:
        ldx     #step_swi
        stx     VEC_SWI
        swi
step_swi:
        ldx     #VEC_SWI
        stx     VEC_SWI
        swi                     ; halt to system

getchar:
        ldaa    ACIA_status
        bita    #RDRF_bm
        beq     getchar
        ldaa    ACIA_data
        rts

putchar:
        ldab    ACIA_status
        bitb    #TDRE_bm
        beq     putchar
        staa    ACIA_data
        rts
