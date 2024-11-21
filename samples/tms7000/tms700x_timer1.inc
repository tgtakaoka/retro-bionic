        *** -*- mode: asm; mode: flyspell-prog; -*-
        *** Timer 1 (Not in TMS70C02)
VEC_INT2:       equ     >FFFA   Timer 1
        *** IOCNT0
INT2_F: equ     ?00001000       INT2 flag
INT2_E: equ     ?00000100       INT2 enable
        *** Timer 1
T1DATA: equ     >0102   Timer 1 data
T1CTL:  equ     >0103   Timer 1 control
T1_START:       equ     ?10000000       1=Reload, 0=Stop
T1_SOURCE:      equ     ?01000000       1=External A7, 0=fosc/16
T1_PRE_gm:      equ     ?00011111       Prescale reload register
