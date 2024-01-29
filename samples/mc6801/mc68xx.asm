        cpu     6801
        include "mc6801.inc"

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     $FFF6           ; MC68HC11 SWI vector
        fdb     $FFF6

        org     VEC_RESET
        fdb     main

        org     $1000
main:
        lds     #main-1
        bsr     mc68xx
        swi

        include "mc68xx.inc"
