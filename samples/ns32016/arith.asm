;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "ns32016.inc"
        include "../mc6800/mc6850.inc"
        include "vectors.inc"

stack:  equ     0x1000

        org     0x20
dispatch:
        .space  4*VEC_BPT
        .word   module, halt_to_system ; BPT

        org     0x100
init:
        lprd    sp, stack
        lprd    intbase, dispatch
        lprw    mod, module
        setcfg  []
        movb    CDS_RESET_gc, @ACIA_control ; Master reset
        movb    WSB_8N1_gc, @ACIA_control   ; 8 bits + No Parity + 1 Stop Bits

        bsr     arith
halt_to_system:
        bpt

;;; Put character
;;; @param R0
putchar:
        tbitb   1, @ACIA_status ; TDRE
        bfc     putchar
        movb    r0, @ACIA_data
        ret     0

newline:
        save    [r0]
        movb    0x0D, r0
        bsr     putchar
        movb    0x0A, r0
        bsr     putchar
        restore [r0]
        ret     0

putspace:
        save    [r0]
        movb    ' ', r0
        bsr     putchar
        restore [r0]
        ret     0

;;; Print "v1 op v2"
;;; @param R4 v1
;;; @param R5 v2
;;; @param R0 op
;;; @return R1 v1
;;; @clobber R0
expr:
        save    [r0]
        movw    r4, r1
        bsr     print_int16
        bsr     putspace
        restore [r0]
        bsr     putchar
        bsr     putspace
        movw    r5, r1
        bsr     print_int16
        movw    r4, r1
        ret     0

;;; Print " = v1\n"
;;; @param R1 v1
;;; @clobber R0 R1
answer:
        bsr     putspace
        movb    '=', r0
        bsr     putchar
        bsr     putspace
        bsr     print_int16
        br      newline

;;; Compare and print "v1 rel v2\n"
;;; @param R4 v1
;;; @param R5 v2
;;; @clobber R0 R1
comp:
        cmpw    r4, r5
        beq     comp_eq
        bgt     comp_gt         ; v1 > v2
        movb    '<', r0
        br      comp_out
comp_gt:
        movb    '>', r0
        br      comp_out
comp_eq:
        movb    '=', r0
comp_out:
        bsr     expr
        br      newline

        include "arith.inc"

        org     0x1000
arith:
        movw    18000, r4
        movw    28000, r5
        movb    '+', r0
        bsr     expr
        addw    r5, r1
        bsr     answer          ; -19536
        movw    18000, r4
        movw    -18000, r5
        movb    '+', r0
        bsr     expr
        addw    r5, r1
        bsr     answer          ; 0
        movw    -18000, r4
        movw    -18000, r5
        movb    '+', r0
        bsr     expr
        addw    r5, r1
        bsr     answer          ; 29536
        movw    -18000, r4
        movw    -28000, r5
        movb    '-', r0
        bsr     expr
        subw    r5, r1
        bsr     answer          ; 10000
        movw    100, r4
        movw    300, r5
        movb    '*', r0
        bsr     expr
        mulw    r5, r1
        bsr     answer          ; 30000
        movw    300, r4
        movw    -200, r5
        movb    '*', r0
        bsr     expr
        mulw    r5, r1
        bsr     answer          ; 5536
        movw    100, r4
        movw    -300, r5
        movb    '*', r0
        bsr     expr
        mulw    r5, r1
        bsr     answer          ; -30000
        movw    -200, r4
        movw    -100, r5
        movb    '*', r0
        bsr     expr
        mulw    r5, r1
        bsr     answer          ; 20000
        movw    30000, r4
        movw    100, r5
        movb    '/', r0
        bsr     expr
        quow    r5, r1
        bsr     answer          ; 300
        movw    -200, r4
        movw    100, r5
        movb    '/', r0
        bsr     expr
        quow    r5, r1
        bsr     answer          ; -2
        movw    -30000, r4
        movw    -200, r5
        movb    '/', r0
        bsr     expr
        quow    r5, r1
        bsr     answer          ; 150
        movw    -30000, r4
        movw    78, r5
        movb    '/', r0
        bsr     expr
        quow    r5, r1
        bsr     answer          ; -384
        movw    -48, r4
        movw    30, r5
        bsr     comp
        movw    30, r4
        movw    -48, r5
        bsr     comp
        movw    5000, r4
        movw    4000, r5
        bsr     comp
        movw    5000, r4
        movw    5000, r5
        bsr     comp
        movw    4000, r4
        movw    5000, r5
        bsr     comp
        movw    -5000, r4
        movw    -4000, r5
        bsr     comp
        movw    -5000, r4
        movw    -5000, r5
        bsr     comp
        movw    -4000, r4
        movw    -5000, r5
        bsr     comp
        movw    32700, r4
        movw    32600, r5
        bsr     comp
        movw    32700, r4
        movw    32700, r5
        bsr     comp
        movw    32600, r4
        movw    32700, r5
        bsr     comp
        movw    -32700, r4
        movw    -32600, r5
        bsr     comp
        movw    -32700, r4
        movw    -32700, r5
        bsr     comp
        movw    -32600, r4
        movw    -32700, r5
        bsr     comp
        movw    18000, r4
        movw    -28000, r5
        bsr     comp
        movw    18000, r4
        movw    18000, r5
        bsr     comp
        movw    -28000, r4
        movw    18000, r5
        bsr     comp
        ret     0
