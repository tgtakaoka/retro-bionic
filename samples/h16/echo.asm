        include "hd641016.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FFDF00
        include "../mc6800/mc6850.inc"

stack:  equ     $1000

        org     VEC_RESET
        .data.l    stack
        .data.l    initialize

        org     $1000
initialize:
        mov.b   #CDS_RESET_gc, @ACIA_control ; Master reset
        mov.b   #WSB_8N1_gc, @ACIA_control   ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
loop:   bsr     getchar
        tst.b   r0
        beq     halt_to_system
echo:   bsr     putchar
        cmp.b   #$0D, r0
        bne     loop
        mov.b   #$0A, r0
        bra     echo
halt_to_system:
        mov.l   #step_trap, @VEC_TRAPA15
        trapa   #15
step_trap:
        mov.l   #VEC_TRAPA15, @VEC_TRAPA15
        trapa   #15             ; halt to system

getchar:
        btst.b  #0, @ACIA_status        ; RDRF
        beq     getchar
        mov.b   @ACIA_data, r0
        rts

putchar:
        btst.b  #1, @ACIA_status        ; TDRE
        beq     putchar
        mov.b   r0, @ACIA_data
        rts
