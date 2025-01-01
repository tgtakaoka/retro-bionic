*** -*- mode: asm; mode: flyspell-prog; -*-
        *** TMS99105
        include "tms9900.inc"
        cpu     tms99105
        *** Vector
VEC_INT2:       equ     >0008   ; INT2
VEC_INT3:       equ     >000C   ; INT3
VEC_INT5:       equ     >0014   ; INT5
VEC_INT6:       equ     >0018   ; INT6
VEC_INT7:       equ     >001C   ; INT7
VEC_INT8:       equ     >0020   ; INT8
VEC_INT9:       equ     >0024   ; INT9
VEC_INT10:      equ     >0028   ; INT10
VEC_INT11:      equ     >002C   ; INT11
VEC_INT12:      equ     >0030   ; INT12
VEC_INT13:      equ     >0034   ; INT13
VEC_INT14:      equ     >0038   ; INT14
VEC_INT15:      equ     >003C   ; INT15
