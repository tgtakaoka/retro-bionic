*** -*- mode: asm; mode: flyspell-prog; -*-
        include "tms99110.inc"

*** MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     >0080
        include "mc6850.inc"

        org     VEC_RESET
        data    workspace
        data    initialize

        org     VEC_INT2
        data    workspace_illop
        data    illop

        org     VEC_XOP15
        data    VEC_XOP15       for halt to system
        data    VEC_XOP15

        org     >0100
workspace:
        bss     2*16
workspace_illop:
        bss     2*16

illop:
        xop     R15,15          halt to system

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
        blsk    R10, arith
        xop     R15,15          halt to system

newline:
        li      R0, hibyte(>0D)
        blsk    R10, putchar
        li      R0, hibyte(>0A)
        blsk    R10, putchar
        bind    *R10+

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
        bind    *R10+

*** Print "A op B"
*** @param R0 op letter
*** @param R1 signed 16-bit integer A
*** @param R2 signed 16-bit integer B
*** @return R0:R1 single 32-bit float A
*** @return R2:R3 single 32-bit float B
expr:
        dect    R10
        mov     R0, *R10        Push R0
        mov     R1, R0
        blsk    R10, print_int16
        blsk    R10, putspace
        mov     *R10+, R0       Pop R0
        blsk    R10, putchar    Print op
        blsk    R10, putspace
        mov     R2, R0
        blsk    R10, print_int16
        dect    R10
        mov     R1, *R10        Push R1
        cir     R2              convert R2 into R0:R1
        str     R2              move R1:R2 to R2:F3
        mov     *R10+, R1       Pop R1
        cir     R1              convert R1 into R0:R1
        bind    *R10+

*** Print " = V\n"
*** @param R0:R1 single 32-bit float V
*** @clobber R0:R1
answer:
        dect    R10
        mov     R0, *R10        Push R0
        blsk    R10, putspace
        li      R0, hibyte('=')
        blsk    R10, putchar
        blsk    R10, putspace
        mov     *R10+, R0       Pop R0
        cri                     convert R0:R1 to R0
        blsk    R10, print_int16
        blsk    R10, newline
        bind    *R10+

*** Print "A rel B"
*** @param R1 signed 16-bit integer A
*** @param R2 signed 16-bit integer B
*** @clobber R0 R1 R2 R2
comp:
        dect    R10
        mov     R2, *R10        Push R2
        dect    R10
        mov     R1, *R10        Push R1
        cir     R2              convert R2 into R0:R1
        str     R2              move R0:R1 to R2:R3
        mov     *R10, R1        Restore R1
        cir     R1              convert R1 into R0:R1
        cr      R0, R2          compare single float
        jeq     comp_eq
        jgt     comp_gt
comp_lt:
        li      R0, hibyte('<')
        jmp     comp_out
comp_eq:
        li      R0, hibyte('=')
        jmp     comp_out
comp_gt:
        li      R0, hibyte('>')
comp_out:
        mov     *R10+, R1       Pop R1
        mov     *R10+, R2       Pop R2
        blsk    R10, expr
        blsk    R10, newline
        bind    *R10+

arith:
        li      R1, 18000
        li      R2, 28000
        li      R0, hibyte('+')
        blsk    R10, expr
        ar      R2              R0:R1+=R2:R3
        blsk    R10, answer     ; -19536

        li      R1, 18000
        li      R2, -18000
        li      R0, hibyte('+')
        blsk    R10, expr
        ar      R2              R0:R1+=R2:R3
        blsk    R10, answer     ; 0

        li      R1, -18000
        li      R2, -18000
        li      R0, hibyte('+')
        blsk    R10, expr
        ar      R2              R0:R1+=R2:R3
        blsk    R10, answer     ; 29536

        li      R1, -18000
        li      R2, -28000
        li      R0, hibyte('-')
        blsk    R10, expr
        sr      R2              R0:R1-=R2:R3
        blsk    R10, answer     ; 10000

        li      R1, 18000
        li      R2, -18000
        li      R0, hibyte('-')
        blsk    R10, expr
        sr      R2              R0:R1-=R2:R3
        blsk    R10, answer     ; -29536

        li      R1, -28000
        li      R2, -18000
        li      R0, hibyte('-')
        blsk    R10, expr
        sr      R2              R0:R1-=R2:R3
        blsk    R10, answer     ; -10000

        li      R1, 100
        li      R2, 300
        li      R0, hibyte('*')
        blsk    R10, expr
        mov     R2, R0
        mr      R2              R0:R1*=R2:R3
        blsk    R10, answer     ; 30000

        li      R1, 200
        li      R2, 100
        li      R0, hibyte('*')
        blsk    R10, expr
        mov     R2, R0
        mr      R2              R0:R1*=R2:R3
        blsk    R10, answer     ; 20000

        li      R1, 300
        li      R2, -200
        li      R0, hibyte('*')
        blsk    R10, expr
        mov     R2, R0
        mr      R2              R0:R1*=R2:R3
        blsk    R10, answer     ; 5536

        li      R1, 100
        li      R2, -300
        li      R0, hibyte('*')
        blsk    R10, expr
        mov     R2, R0
        mr      R2              R0:R1*=R2:R3
        blsk    R10, answer     ; -30000

        li      R1, -200
        li      R2, -100
        li      R0, hibyte('*')
        blsk    R10, expr
        mov     R2, R0
        mr      R2              R0:R1*=R2:R3
        blsk    R10, answer     ; 20000

        li      R1, 30000
        li      R2, 100
        li      R0, hibyte('/')
        blsk    R10, expr
        dr      R2              R0:R1/=R2:R3
        blsk    R10, answer     ; 300

        li      R1, -200
        li      R2, 100
        li      R0, hibyte('/')
        blsk    R10, expr
        dr      R2              R0:R1/=R2:R3
        blsk    R10, answer     ; -2

        li      R1, -30000
        li      R2, -200
        li      R0, hibyte('/')
        blsk    R10, expr
        dr      R2              R0:R1/=R2:R3
        blsk    R10, answer     ; 150

        li      R1, -30000
        li      R2, 78
        li      R0, hibyte('/')
        blsk    R10, expr
        dr      R2              R0:R1/=R2:R3
        blsk    R10, answer     ; -384

        li      R1, 5000
        li      R2, 4000
        blsk    R10, comp

        li      R1, 5000
        li      R2, 5000
        blsk    R10, comp

        li      R1, 4000
        li      R2, 5000
        blsk    R10, comp

        li      R1, -5000
        li      R2, -4000
        blsk    R10, comp

        li      R1, -5000
        li      R2, -5000
        blsk    R10, comp

        li      R1, -4000
        li      R2, -5000
        blsk    R10, comp

        li      R1, 32700
        li      R2, 32600
        blsk    R10, comp

        li      R1, 32700
        li      R2, 32700
        blsk    R10, comp

        li      R1, 32600
        li      R2, 32700
        blsk    R10, comp

        li      R1, -32700
        li      R2, -32600
        blsk    R10, comp

        li      R1, -32700
        li      R2, -32700
        blsk    R10, comp

        li      R1, -32600
        li      R2, -32700
        blsk    R10, comp

        li      R1, 18000
        li      R2, -28000
        blsk    R10, comp

        li      R1, -28000
        li      R2, -28000
        blsk    R10, comp

        li      R1, -28000
        li      R2, 18000
        blsk    R10, comp

        bind    *R10+

        include "arith.inc"
