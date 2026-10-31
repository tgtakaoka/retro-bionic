;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "ns32016.inc"
        include "../mc6800/mc6850.inc"
        include "vectors.inc"

stack:  equ     0x1000

        org     0x20
dispatch:
        .space  4*VEC_BPT
        .word   module, halt_to_system ; BPT

        org     0x100
init:
        lprd    sp, stack
        lprd    intbase, dispatch
        lprw    mod, module
        setcfg  []
        movb    CDS_RESET_gc, @ACIA_control ; Master reset
        movb    WSB_8N1_gc, @ACIA_control   ; 8 bits + No Parity + 1 Stop Bits
receive_loop:
        movb    @ACIA_status, r0
        andb    RDRF_bm, r0
        cmpqb   0, r0
        beq     receive_loop
receive_data:
        movb    @ACIA_data, r1
        cmpqb   0, r1
        beq     halt_to_system
transmit_loop:
        movb    @ACIA_status, r0
        andb    TDRE_bm, r0
        cmpqb   0, r0
        beq     transmit_loop
transmit_data:
        movb    r1, @ACIA_data
        cmpb    0x0D, r1
        bne     receive_loop
        movb    0x0A, r1
        br      transmit_loop
halt_to_system:
        bpt
