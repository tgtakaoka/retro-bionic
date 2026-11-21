;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; CP/M 2.2 BIOS on 16-bit ports, for the Z180, Z280 and Z380.
        cpu     8080
        include "../i8080/cpm80.inc"

        org     BIOS
        include "../i8080/cpm80bios.inc"
        include "../i8080/cpm80load.inc"
        include "cpm80io.inc"
