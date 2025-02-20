        *** -*- mode: asm; mode: flyspell-prog; -*-
        *** TMS7000
        *** Status Register
ST_CARRY:       equ  ?10000000  set to 1 if carry occurred
ST_NEGATIVE:    equ  ?01000000  set to 1 if result is negative
ST_ZERO:        equ  ?00100000  set to 1 if result is zero
ST_INT:         equ  ?00010000  if 1, interrupt is enabled
        *** Vector
VEC_INT3:       equ     >FFF8   #INT3 interrupt
VEC_INT1:       equ     >FFFC   #INT1 interrupt
VEC_RESET:      equ     >FFFE   #RESET vector
        *** Trap Vector
VEC_TRAP0:      equ     >FFFE
VEC_TRAP1:      equ     >FFFC
VEC_TRAP2:      equ     >FFFA
VEC_TRAP3:      equ     >FFF8
VEC_TRAP4:      equ     >FFF6
VEC_TRAP5:      equ     >FFF4
VEC_TRAP6:      equ     >FFF2
VEC_TRAP7:      equ     >FFF0
VEC_TRAP8:      equ     >FFEE
VEC_TRAP9:      equ     >FFEC
VEC_TRAP10:     equ     >FFEA
VEC_TRAP11:     equ     >FFE8
VEC_TRAP12:     equ     >FFE6
VEC_TRAP13:     equ     >FFE4
VEC_TRAP14:     equ     >FFE2
VEC_TRAP15:     equ     >FFE0
VEC_TRAP16:     equ     >FFDE
VEC_TRAP17:     equ     >FFDC
VEC_TRAP18:     equ     >FFDA
VEC_TRAP19:     equ     >FFD8
VEC_TRAP20:     equ     >FFD6
VEC_TRAP21:     equ     >FFD4
VEC_TRAP22:     equ     >FFD2
VEC_TRAP23:     equ     >FFD0
        *** Internal Peripherals
IOCNT0: equ     >0100   I/O Control register
MEMMODE_gm:     equ     ?11000000       Memory mode
        * 0=Single Chip, 1=Peripheral Expansion, 2=Full Expansion
INT3_F: equ     ?00100000       #INT3 flag
INT3_E: equ     ?00010000       #INT3 enable
INT1_F: equ     ?00000010       #INT1 flag
INT1_E: equ     ?00000001       #INT1 enable
        *** I/O Ports
APORT:  equ     >0104   Port A data
BPORT:  equ     >0106   Port B data
CPORT:  equ     >0108   Port C data
CDDR:   equ     >0109   Port C data direction register
DPORT:  equ     >010A   Port D data
DDDR:   equ     >010B   Port D data direction register
