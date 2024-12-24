*** -*- mode: asm; mode: flyspell-prog; -*-
        *** TMS9900
        cpu     tms9900
        *** Status Register
ST_LGT:         equ     ?1000000000000000 ; Logical Greater Than
ST_AGT:         equ     ?0100000000000000 ; Arithmetic Greater Than
ST_EQ:          equ     ?0010000000000000 ; Equal
ST_C:           equ     ?0001000000000000 ; Carry
ST_OV:          equ     ?0000100000000000 ; Overflow
ST_OP:          equ     ?0000010000000000 ; Parity
ST_X:           equ     ?0000001000000000 ; XOP
ST_INT_gm:      equ     ?0000000000001111 ; Interrupt mask
        *** Vector
VEC_RESET:      equ     >0000   ; Reset
VEC_INT1:       equ     >0004   ; INT1
VEC_INT4:       equ     >0010   ; INT4
VEC_XOP0:       equ     >0040   ; XOP 0
VEC_XOP1:       equ     >0044   ; XOP 1
VEC_XOP2:       equ     >0048   ; XOP 2
VEC_XOP3:       equ     >004C   ; XOP 3
VEC_XOP4:       equ     >0050   ; XOP 4
VEC_XOP5:       equ     >0054   ; XOP 5
VEC_XOP6:       equ     >0058   ; XOP 6
VEC_XOP7:       equ     >005C   ; XOP 7
VEC_XOP8:       equ     >0060   ; XOP 8
VEC_XOP9:       equ     >0064   ; XOP 9
VEC_XOP10       equ     >0068   ; XOP 10
VEC_XOP11:      equ     >006C   ; XOP 11
VEC_XOP12:      equ     >0070   ; XOP 12
VEC_XOP13:      equ     >0074   ; XOP 13
VEC_XOP14:      equ     >0078   ; XOP 14
VEC_XOP15:      equ     >007C   ; XOP 15
