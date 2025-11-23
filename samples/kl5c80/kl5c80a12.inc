;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "../z80/z80.inc"
;;; MMU
BBR1:   equ     00H
BR1:    equ     01H
BBR2:   equ     02H
BR2:    equ     03H
BBR3:   equ     04H
BR3:    equ     05H
BBR4:   equ     06H
BR4:    equ     07H
;;; Interrupt controller
LERL:   equ     34H        ; W/_: Level/Edge Register (LER0~LER7)
LERH:   equ     35H        ; W/_: Level/Edge Register (LER8~LER15)
PGRL:   equ     LERL       ; W/_: Priority Group Register (PGR0~PGR7)
PGRH:   equ     LERH       ; W/_: Priority Group Register (PGR8~PGR15)
ISRL:   equ     LERL       ; R/_: In Service Register (ISR0~ISR7)
ISRH:   equ     LERH       ; R/_: In Service Register (ISR8~ISR15)
IMRL:   equ     36H        ; R/W: Interrupt Mask Register (IMR0~IMR7)
IMRH:   equ     37H        ; R/W: Interrupt Mask Register (IMR8~IMR15)
IVR:    equ     IMRH       ; _/W: Interrupt Vector Register (IVR5~IVR7)
