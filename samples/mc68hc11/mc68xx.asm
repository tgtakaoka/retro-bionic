        include "mc68hc11.inc"

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     main

        org     $9000
main:
        lds     #main-1
        bsr     mc68xx
        swi

        include "../mc6800/mc68xx.inc"
