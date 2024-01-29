        cpu     68hc11
        include "mc68hc11.inc"

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     main

        org     $1000
main:
        lds     #main-1
        bsr     mc68xx
        swi

        include "mc68xx.inc"
