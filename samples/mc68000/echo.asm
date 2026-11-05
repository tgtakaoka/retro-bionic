        include "mc68000.inc"

stack:  equ     $1000           ; SSP: a push decrements it first

        org     VEC_RESET
        dc.l    stack
        dc.l    initialize

        org     $1000
initialize:
        move.b  #CDS_RESET_gc,(ACIA_control).w ; Master reset
        move.b  #WSB_8N1_gc,(ACIA_control).w   ; 8 bits + No Parity + 1 Stop Bits
                                               ; Transmit, Receive interrupts disabled

loop:   bsr.s   getchar
        tst.b   d0
        beq.s   halt_to_system
echo:   bsr.s   putchar
        cmp.b   #$0D,d0
        bne.s   loop
        move.b  #$0A,d0
        bra.s   echo
halt_to_system:
        illegal                 ; halt to system

getchar:
        btst    #0,(ACIA_status).w ; RDRF
        beq.s   getchar
        move.b  (ACIA_data).w,d0
        rts

putchar:
        btst    #1,(ACIA_status).w ; TDRE
        beq.s   putchar
        move.b  d0,(ACIA_data).w
        rts
