*** -*- mode: asm; mode: flyspell-prog; -*-
        *** TMS9981
        include "tms9900.inc"
        cpu     tms9900
        *** Vector
VEC_INT2:       equ     >0008   ; INT2
VEC_INT3:       equ     >000C   ; INT3
VEC_NMI:        equ     >3FFC   ; LOAD (NMI)
