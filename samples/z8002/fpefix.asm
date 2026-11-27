;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; A fix to Zilog's Z8070 emulator, FPE.O, which xout-link.py patches
;;; in: for a segmented program's long-offset address, memadrs takes the
;;; offset from R2, but getiword returns the word in R3, as the first
;;; address word's path shows.
        cpu     z8002
        include "fpesys.inc"

        org     fpepu553+6
        ld      r7, r3          ; was ld r7, r2
