;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "i80296.inc"
        org     ORG_RESET
        include "extint0.inc"
ORG_CODE:
        include "../i8096/echoitr.inc"
