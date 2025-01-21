*** -*- mode: asm; mode: flyspell-prog; -*-
*** MN1610
        cpu     mn1610
*** Status Register
STR_E:          equ     0       Extension
STR_OV:         equ     2       Overflow
STR_IRQ0:       equ     5       IRQ0 Mask
STR_IRQ1:       equ     6       IRQ1 Mask
STR_IRQ2:       equ     7       IRQ2 Mask
*** Interrupt vector and save area
OPSW0:          equ     X'0000' IRQ0 old STR:IC
OPSW1:          equ     X'0002' IRQ1 old STR:IC
OPSW2:          equ     X'0004' IRQ2 old STR:IC
OPSW3:          equ     X'0006'
NPSW0:          equ     X'0100' IRQ0 vector STR:IC
NPSW1:          equ     X'0102' IRQ1 vector STR:IC
NPSW2:          equ     X'0104' IRQ2 vector STR:IC
NPSW3:          equ     X'0106'
*** Reset PSW (see debugger RegsMn1613::reset)
RESET_PSW:      equ     X'0108'
*** Zero page
ZERO_PAGE:      equ     X'0008'

hi:     function        v, (v >> 8)
lo:     function        v, (v & X'FF')
