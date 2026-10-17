;;; -*- mode: asm; mode: flyspell-prog; -*-
        .include "tms320c25.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   .equ    4
        .include "mc6850.inc"

;;; Data memory
        .org    B0
work:   .bss    1
char:   .bss    1
vA:     .bss    1
vB:     .bss    1
expr_op:
comp_rel:       .bss    1
R0:     .bss    1
R1:     .bss    1
R2:     .bss    1
print_buf:      .bss    8

        .org    ORG_RESET
        b       initialize

        .org    100H
initialize:
        rovm                    ; reset leaves OVM unchanged
        ldpk    PAGE_B0
;;; Initialize ACIA
        lack    CDS_RESET_gc
        sacl    work
        out     work,ACIA_control ; Master reset
        lack    WSB_8N1_gc
        sacl    work
        out     work,ACIA_control ; 8 bits + No Parity + 1 Stop Bits
        b       arith

newline:
        lack    0DH
        call    putchar
        lack    0AH
        call    putchar
        ret

putspace:
        lack    ' '
        call    putchar
        ret

putchar:
        sacl    char
putchar_loop:
        in      work,ACIA_status
        lac     work
        andk    TDRE_bm
        bz      putchar_loop
        out     char,ACIA_data
        zals    char
        ret

;;; Print "A op B"
;;; @param vA A
;;; @param vB B
;;; @param ACC op
;;; @return T=A
;;; @clobber R0 R1 R2
expr:
        sacl    expr_op
        lac     vA
        call    print_int16     ; print A
        call    putspace
        zals    expr_op         ; print op
        call    putchar
        call    putspace
        lac     vB
        call    print_int16     ; print B
        ret

;;; Print " = v\n"
;;; @param ACC v
;;; @clobber R0 R1 R2
answer:
        sacl    vA
        call    putspace
        lack    '='
        call    putchar
        call    putspace
        lac     vA
        call    print_int16     ; print v
        call    newline
        ret

;;; Print "A rel B"
;;; @param vA A
;;; @param vB B
;;; @clobber R0 R1 R2
comp:
        lac     vA
        sub     vB
        bz      comp_eq
        blz     comp_lt
comp_gt:
        lack    '>'
        b       comp_put
comp_eq:
        lack    '='
        b       comp_put
comp_lt:
        lack    '<'
comp_put:
        sacl    comp_rel
        lac     vA
        call    print_int16     ; print A
        call    putspace
        zals    comp_rel
        call    putchar         ; print rel
        call    putspace
        lac     vB
        call    print_int16     ; print B
        call    newline
        ret

arith:
        lalk    18000
        sacl    vA
        lalk    28000
        sacl    vB
        lack    '+'
        call    expr
        lac     vA
        add     vB
        call    answer          ; -19536

        lalk    18000
        sacl    vA
        lalk    -18000
        sacl    vB
        lack    '+'
        call    expr
        lac     vA
        add     vB
        call    answer          ; 0

        lalk    -18000
        sacl    vA
        lalk    -18000
        sacl    vB
        lack    '+'
        call    expr
        lac     vA
        add     vB
        call    answer          ; 29536

        lalk    -18000
        sacl    vA
        lalk    -28000
        sacl    vB
        lack    '-'
        call    expr
        lac     vA
        sub     vB
        call    answer          ; 10000

        lalk    18000
        sacl    vA
        lalk    -18000
        sacl    vB
        lack    '-'
        call    expr
        lac     vA
        sub     vB
        call    answer          ; -29536

        lalk    -28000
        sacl    vA
        lalk    -18000
        sacl    vB
        lack    '-'
        call    expr
        lac     vA
        sub     vB
        call    answer          ; -10000

        lalk    100
        sacl    vA
        lalk    300
        sacl    vB
        lack    '*'
        call    expr
        lt      vA
        mpy     vB
        pac
        call    answer          ; 30000

        lalk    200
        sacl    vA
        lalk    100
        sacl    vB
        lack    '*'
        call    expr
        lt      vA
        mpy     vB
        pac
        call    answer          ; 20000

        lalk    300
        sacl    vA
        lalk    -200
        sacl    vB
        lack    '*'
        call    expr
        lt      vA
        mpy     vB
        pac
        call    answer          ; 5536

        lalk    100
        sacl    vA
        lalk    -300
        sacl    vB
        lack    '*'
        call    expr
        lt      vA
        mpy     vB
        pac
        call    answer          ; -30000

        lalk    -200
        sacl    vA
        lalk    -100
        sacl    vB
        lack    '*'
        call    expr
        lt      vA
        mpy     vB
        pac
        call    answer          ; 20000

        lalk    30000
        sacl    vA
        lalk    100
        sacl    vB
        lack    '/'
        call    expr
        lac     vA
        sacl    R1
        lac     vB
        sacl    R2
        call    div16
        call    answer          ; 300

        lalk    -200
        sacl    vA
        lalk    100
        sacl    vB
        lack    '/'
        call    expr
        lac     vA
        sacl    R1
        lac     vB
        sacl    R2
        call    div16
        call    answer          ; -2

        lalk    -30000
        sacl    vA
        lalk    -200
        sacl    vB
        lack    '/'
        call    expr
        lac     vA
        sacl    R1
        lac     vB
        sacl    R2
        call    div16
        call    answer          ; 150

        lalk    -30000
        sacl    vA
        lalk    78
        sacl    vB
        lack    '/'
        call    expr
        lac     vA
        sacl    R1
        lac     vB
        sacl    R2
        call    div16
        call    answer          ; -384

        lalk    5000
        sacl    vA
        lalk    4000
        sacl    vB
        call    comp

        lalk    5000
        sacl    vA
        lalk    5000
        sacl    vB
        call    comp

        lalk    4000
        sacl    vA
        lalk    5000
        sacl    vB
        call    comp

        lalk    -5000
        sacl    vA
        lalk    -4000
        sacl    vB
        call    comp

        lalk    -5000
        sacl    vA
        lalk    -5000
        sacl    vB
        call    comp

        lalk    -4000
        sacl    vA
        lalk    -5000
        sacl    vB
        call    comp

        lalk    32700
        sacl    vA
        lalk    32600
        sacl    vB
        call    comp

        lalk    32700
        sacl    vA
        lalk    32700
        sacl    vB
        call    comp

        lalk    32600
        sacl    vA
        lalk    32700
        sacl    vB
        call    comp

        lalk    -32700
        sacl    vA
        lalk    -32600
        sacl    vB
        call    comp

        lalk    -32700
        sacl    vA
        lalk    -32700
        sacl    vB
        call    comp

        lalk    -32600
        sacl    vA
        lalk    -32700
        sacl    vB
        call    comp

        lalk    18000
        sacl    vA
        lalk    -28000
        sacl    vB
        call    comp

        lalk    -28000
        sacl    vA
        lalk    -28000
        sacl    vB
        call    comp

        lalk    -28000
        sacl    vA
        lalk    18000
        sacl    vB
        call    comp

        .word   HALT

        .include "arith.inc"
