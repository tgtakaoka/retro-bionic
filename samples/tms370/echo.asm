;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "tms370.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     10F0H
        include "mc6850.inc"

        org     VEC_RESET
        .word   initialize

        org     VEC_TRAP15
        .word   VEC_TRAP15

        org     2000H
initialize:
        mov     #CDS_RESET_gc, ACIA_control ; Master reset
        mov     #WSB_8N1_gc, ACIA_control ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
loop:   call    getchar
        tst     A
        jeq     halt_to_system
echo:   call    putchar
        cmp     #0DH, A
        jne     loop
        mov     #0AH, A
        jmp     echo
halt_to_system:
        trap    15

getchar:
        btjz    #RDRF_bm, ACIA_status, getchar
        mov     ACIA_data, A
        rts

putchar:
        btjz    #TDRE_bm, ACIA_status, putchar
        mov     A, ACIA_data
        rts

;;; local Variables:
;;; mode: asm
;;; mode: flyspell-prog
;;; comment-start: ";"
;;; End:
