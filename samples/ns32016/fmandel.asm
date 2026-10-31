;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "ns32016.inc"
        fpu     ns32081
        include "../mc6800/mc6850.inc"
        include "vectors.inc"

stack:  equ     0x1000

        org     0x20
dispatch:
        .word   module, isr_nvi ; NVI
        .space  4*(VEC_BPT-1)
        .word   module, halt_to_system ; BPT

        org     0x100
init:
        lprd    sp, stack
        lprd    intbase, dispatch
        lprw    mod, module
        setcfg  [f]             ; the NS32081 FPU
        bsr     init_io
        bispsrw PSR_I           ; enable interrupt

loop:
        bsr     fmandel
        bsr     newline
        bsr     queue_wait_empty
        br      loop

        include "fmandel.inc"
        include "io.inc"
