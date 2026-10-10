        include "mc68hc16.inc"

        include "../mc6800/mc6850.inc"

stack:  equ     $1000-2         ; PSHM stores at SP, then decrements it

        org     VEC_RESET
        fdb     (ACIA_BASE>>16)<<8 ; ZK:SK:PK: Z in the ACIA's bank
        fdb     initialize
        fdb     stack
        fdb     ACIA_BASE&$FFFF ; IZ

        org     $0200
initialize:
        clrb
        tbek                    ; data, X and Y in bank 0
        tbxk
        tbyk
        lds     #stack
        ldaa    #CDS_RESET_gc   ; Master reset
        staa    ACIA_control,z
        ldaa    #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        staa    ACIA_control,z

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
        ldaa    ACIA_status,z
        bita    #RDRF_bm
        beq     getchar
        ldaa    ACIA_data,z
        rts

putchar:
        ldab    ACIA_status,z
        bitb    #TDRE_bm
        beq     putchar
        staa    ACIA_data,z
        rts
