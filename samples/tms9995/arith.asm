*** -*- mode: asm; mode: flyspell-prog; -*-
        include "tms9995.inc"

        *** MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     >0080
        include "mc6850.inc"

        org     VEC_RESET
        data    workspace
        data    initialize

        org     VEC_XOP15
        data    VEC_XOP15       for halt to system
        data    VEC_XOP15

        org     >F000
workspace:
        bss     2*16

        org     >2000
stack:          equ     $

hibyte: function v,(v)*256      Hi byte constant

        org     >0200
initialize:
        li      R10, stack
        * Initialize ACIA
        li      R0, hibyte(CDS_RESET_gc)        Master reset
        movb    R0, @ACIA_control
        li      R0, hibyte(WSB_8N1_gc)          8 bits + No Parity + 1 Stop Bits
        movb    R0, @ACIA_control
        bl      @arith
        xop     R15,15          halt to system

newline:
        dect    R10
        mov     R11, *R10               Push return address
        li      R0, hibyte(>0D)
        bl      @putchar
        li      R0, hibyte(>0A)
        bl      @putchar
        mov     *R10+, R11              Pop return address
        rt

putspace:
        li      R0, hibyte(' ')
putchar:
        dect    R10
        mov     R1, *R10                Push R1
putchar_loop:
        movb    @ACIA_status, R1
        andi    R1, hibyte(TDRE_bm)
        jeq     putchar_loop
        movb    R0, @ACIA_data
        mov     *R10+, R1               Pop R1
        rt

        *** Print "R1 op R2"
        *** @params R0 op letter
        *** @clobber R0
expr:
        dect    R10
        mov     R11, *R10       Push return address
        dect    R10
        mov     R0, *R10        Push R0
        mov     R1, R0
        bl      @print_int16
        bl      @putspace
        mov     *R10+, R0       Pop R0
        bl      @putchar        Print op
        bl      @putspace
        mov     R2, R0
        bl      @print_int16
        mov     *R10+, R11      Pop return address
        rt

        *** Print " = R1\n"
        *** @clobber R0
answer:
        dect    R10
        mov     R11, *R10       Push return address
        bl      @putspace
        li      R0, hibyte('=')
        bl      @putchar
        bl      @putspace
        mov     R1, R0
        bl      @print_int16
        bl      @newline
        mov     *R10+, R11      Pop return address
        rt

        *** Print "R1 rel R2"
        *** @clobber R0
comp:
        c       R1, R2
        jeq     comp_eq
        jlt     comp_lt
        jgt     comp_gt
        li      R0, hibyte('?')
        jmp     comp_out
comp_gt:
        li      R0, hibyte('>')
        jmp     comp_out
comp_eq:
        li      R0, hibyte('=')
        jmp     comp_out
comp_lt:
        li      R0, hibyte('<')
comp_out:
        dect    R10
        mov     R11, *R10       Push return address
        bl      @expr
        bl      @newline
        mov     *R10+, R11      Pop return address
        rt

arith:
        dect    R10
        mov     R11, *R10       Push return address

        li      R1, 18000
        li      R2, 28000
        li      R0, hibyte('+')
        bl      @expr
        a       R2, R1          R1+=R2
        bl      @answer         ; -19536

        li      R1, 18000
        li      R2, -18000
        li      R0, hibyte('+')
        bl      @expr
        a       R2, R1          R1+=R2
        bl      @answer         ; 0

        li      R1, -18000
        li      R2, -18000
        li      R0, hibyte('+')
        bl      @expr
        a       R2, R1          R1+=R2
        bl      @answer         ; 29536

        li      R1, -18000
        li      R2, -28000
        li      R0, hibyte('-')
        bl      @expr
        s       R2, R1          R1-=R2
        bl      @answer         ; 10000

        li      R1, 18000
        li      R2, -18000
        li      R0, hibyte('-')
        bl      @expr
        s       R2, R1          R1-=R2
        bl      @answer         ; -29536

        li      R1, -28000
        li      R2, -18000
        li      R0, hibyte('-')
        bl      @expr
        s       R2, R1          R1-=R2
        bl      @answer         ; -10000

        li      R1, 100
        li      R2, 300
        li      R0, hibyte('*')
        bl      @expr
        mov     R2, R0
        mpys    R1
        bl      @answer         ; 30000

        li      R1, 200
        li      R2, 100
        li      R0, hibyte('*')
        bl      @expr
        mov     R2, R0
        mpys    R1
        bl      @answer         ; 20000

        li      R1, 300
        li      R2, -200
        li      R0, hibyte('*')
        bl      @expr
        mov     R2, R0
        mpys    R1
        bl      @answer         ; 5536

        li      R1, 100
        li      R2, -300
        li      R0, hibyte('*')
        bl      @expr
        mov     R2, R0
        mpys    R1
        bl      @answer         ; -30000

        li      R1, -200
        li      R2, -100
        li      R0, hibyte('*')
        bl      @expr
        mov     R2, R0
        mpys    R1
        bl      @answer         ; 20000

        li      R1, 30000
        li      R2, 100
        li      R0, hibyte('/')
        bl      @expr
        bl      @div16
        bl      @answer         ; 300

        li      R1, -200
        li      R2, 100
        li      R0, hibyte('/')
        bl      @expr
        bl      @div16
        bl      @answer         ; -2

        li      R1, -30000
        li      R2, -200
        li      R0, hibyte('/')
        bl      @expr
        bl      @div16
        bl      @answer         ; 150

        li      R1, -30000
        li      R2, 78
        li      R0, hibyte('/')
        bl      @expr
        bl      @div16
        bl      @answer         ; -384

        li      R1, 5000
        li      R2, 4000
        bl      @comp

        li      R1, 5000
        li      R2, 5000
        bl      @comp

        li      R1, 4000
        li      R2, 5000
        bl      @comp

        li      R1, -5000
        li      R2, -4000
        bl      @comp

        li      R1, -5000
        li      R2, -5000
        bl      @comp

        li      R1, -4000
        li      R2, -5000
        bl      @comp

        li      R1, 32700
        li      R2, 32600
        bl      @comp

        li      R1, 32700
        li      R2, 32700
        bl      @comp

        li      R1, 32600
        li      R2, 32700
        bl      @comp

        li      R1, -32700
        li      R2, -32600
        bl      @comp

        li      R1, -32700
        li      R2, -32700
        bl      @comp

        li      R1, -32600
        li      R2, -32700
        bl      @comp

        li      R1, 18000
        li      R2, -28000
        bl      @comp

        li      R1, -28000
        li      R2, -28000
        bl      @comp

        li      R1, -28000
        li      R2, 18000
        bl      @comp

        mov     *R10+, R11      Pop return address
        rt

        include "arith.inc"
