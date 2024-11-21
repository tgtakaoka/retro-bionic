        *** -*- mode: asm; mode: flyspell-prog; -*-
        *** Timer 2 (TMS7001/TMS7002)
VEC_INT5:       equ     >FFF4   Timer 2
        *** IOCNT1
INT5_F: equ     ?1000   INT5 flag
INT5_E: equ     ?0100   INT5 enable
        *** Timer 2
T2_START:       equ     ?10000000       1=Reload, 0=Stop
T2_SOURCE:      equ     ?01000000       1=External A6, 0=fosc/16
T2_CASCADE:     equ     ?00100000       1=source is Timer 1
