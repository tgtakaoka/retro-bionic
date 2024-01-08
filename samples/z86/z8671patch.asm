;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Z8671 SIO patch
;;; http://www.armory.com/~rstevew/Public/Micros/Z8/Z8671-BASIC/Z8671-BASIC_ROM_Main.htm
        cpu     z86c
        option  "reg-alias", "disable"

        include "z8.inc"

        org     %8C
RESET:  setrp   SIO
        srp     #SIO
        clr     IMR
        ;; Stack is on external memory
        ld      P01M, #P01M_P0ADDR LOR P01M_P1DATA
        ld      SPL, #%68
        ld      T0, #12
        ld      PRE0, #(1 SHL PRE0_gp) LOR PRE0_MODULO
        ld      P3M, #P3M_SERIAL LOR P3M_P2PUSHPULL
        ld      TMR, #TMR_LOAD_T0 LOR TMR_ENABLE_T0
        ld      FLAGS, #F_USER2
        setrp   %10
        srp     #%10
        clr     PORT3
        jp      %00B5

        org     %114
        srp     #%00            ; %08

        org     %129
        srp     #%10            ; %13

        org     %17E
        srp     #%10            ; %13

        org     %1FA
        srp     #%00            ; %0C

        org     %20C
        srp     #%10            ; %13

        org     %60F
        srp     #%10            ; %13

        end
