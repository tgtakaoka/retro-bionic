*** -*- mode: asm; mode: flyspell-prog; -*-
        include "tms9900.inc"

        *** MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     >0080
        include "mc6850.inc"

        org     VEC_RESET
        data    workspace
        data    initialize

        org     VEC_XOP15
        data    VEC_XOP15       to halt system
        data    VEC_XOP15

        org     >0100
workspace:
        bss     2*16

hibyte: function v,(v)*256      Hi byte constant

        org     >0200
initialize:
        * Initialize ACIA
        li      R0, hibyte(CDS_RESET_gc)
        movb    R0, @ACIA_control       Master reset
        li      R0, hibyte(WSB_8N1_gc)
        movb    R0, @ACIA_control       8 bits + No Parity + 1 Stop Bits

loop:   bl      @getchar
        soc     R0, R0
        jeq     halt_to_system
echo:   bl      @putchar
        ci      R0, hibyte(>0D) Cariage Return
        jne     loop
        li      R0, hibyte(>0A) Newline
        jmp     echo
halt_to_system:
        xop     R15,15

getchar:
        movb    @ACIA_status, R0
        andi    R0, hibyte(RDRF_bm)
        jeq     getchar
        movb    @ACIA_data, R0  LSB is cleared by andi
        rt

putchar:
        movb    @ACIA_status, R1
        andi    R1, hibyte(TDRE_bm)
        jeq     putchar
        movb    R0, @ACIA_data
        rt
