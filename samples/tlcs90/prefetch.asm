;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "tlcs90.inc"
        cpu     tlcs90

        org     ORG_RESET
        ld      sp, 1000H
        jp      main

        org     ORG_SWI
        halt                    ; halt to system

dir:    equ     0FFF8H

        org     1000H
main:
        ld      hl, 2000H
        ld      de, 3000H
        jp      pointer

direct:
        ld      ix, 0FFFFH
        ld      (dir), 55H
        inc     ix
        incx    (dir)
        inc     ix
        incx    (dir)
        dec     ix
        decx    (dir)
        dec     ix
        decx    (dir)
        ldw     (dir), 1234H
        inc     (dir)
        dec     (dir)
        incw    (dir)
        decw    (dir)
        res     0,(dir)
        set     0,(dir)
        ld      (dir),hl
        swi

pointer:
        inc     (de)
        dec     (de)
        incw    (de)
        decw    (de)
        res     0,(de)
        set     0,(de)
        tset    0,(de)
        rld     (de)
        rrd     (de)
        rlc     (de)
        ld      (de),h
        ldw     (de),5678H
        ld      (de),hl
        add     (de),89H
        swi

block:
        ld      bc, 4
        ldir
        ld      bc, 4
        lddr
        ld      bc, 4
        cpir
        ld      bc, 4
        cpdr
        swi

jump:
        rcf
        jp      c, jump_end
        nop
        nop
        jp      nc, jump_end
        nop
        nop
jump_end:
        swi
