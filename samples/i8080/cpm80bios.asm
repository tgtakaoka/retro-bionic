;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M 2.2 BIOS on 8-bit ports, for the 8080, 8085, Z80, NSC800 and
;;; KL5C80.
        include "i8080.inc"
        include "cpm80.inc"

        org     BIOS
        include "cpm80bios.inc"
        include "cpm80load.inc"
        include "cpm80io.inc"
