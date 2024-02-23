        cpu     6809
        include "mc6809.inc"

        org     VEC_RESET
        fdb     main

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     $1000
main:
        lds     #main
        bsr     mc6x09
        swi

        include "mc6x09.inc"
