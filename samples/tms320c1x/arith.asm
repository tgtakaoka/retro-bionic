;;; -*- mode: asm; mode: flyspell-prog; -*-
        .include "tms320c15.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   .equ    4
        .include "mc6850.inc"

;;; Data memory
        .org    PAGE0
zero:   .bss    1
one:    .bss    1
work:   .bss    1
char:   .bss    1
putchar_ret:    .bss    1
newline_ret:
putspace_ret:   .bss    1
vA:     .bss    1
vB:     .bss    1
expr_op:
comp_rel:       .bss    1
expr_ret:
answer_ret:
comp_ret:       .bss    1
R0:     .bss    1
R1:     .bss    1
R2:     .bss    1
print_int16_ret:
        .bss    1
udiv16_ret:     .bss    1
divu16_ret:     .bss    1
div16_ret:      .bss    1
print_buf:      .bss    8

        .org    ORG_RESET
        b       initialize

        .org    100H
initialize:
        ldpk    0
        lack    0
        sacl    zero
        lack    1
        sacl    one
;;; Initialize ACIA
        lack    CDS_RESET_gc
        sacl    work
        out     work,ACIA_control ; Master reset
        lack    WSB_8N1_gc
        sacl    work
        out     work,ACIA_control ; 8 bits + No Parity + 1 Stop Bits
        b       arith

newline:
        pop                     ; return address
        sacl    newline_ret
        lack    0DH
        call    putchar
        lack    0AH
        call    putchar
        zals    newline_ret
        push                    ; return address
        ret

putspace:
        pop                     ; return address
        sacl    putspace_ret
        lack    ' '
        call    putchar
        zals    putspace_ret
        push                    ; return address
        ret

putchar:
        sacl    char
        pop                     ; return address
        sacl    putchar_ret
putchar_loop:
        in      work,ACIA_status
        lack    TDRE_bm
        and     work
        bz      putchar_loop
        out     char,ACIA_data
        zals    putchar_ret
        push                    ; return address
        ret

;;; Print "A op B"
;;; @params .word A
;;; @params .word B
;;; @params .word op
;;; @return R1=A
;;; @return R2=B
;;; @return ACC=A
;;; @return T=A
;;; @clobber R0 R1 R2
expr:
        pop                     ; return address
        tblr    vA              ; load A
        add     one
        tblr    vB              ; load B
        add     one
        tblr    expr_op         ; load op
        add     one
        sacl    expr_ret
        lac     vA
        call    print_int16     ; print A
        call    putspace
        zals    expr_op         ; print op
        call    putchar
        call    putspace
        lac     vB
        call    print_int16     ; print B
        zals    expr_ret
        push                    ; return address
        lac     vB
        sacl    R2              ; R2=B
        lac     vA              ; ACC=A
        sacl    R1              ; R1=A
        lt      vA              ; T=A
        ret

;;; Print " = v\n"
;;; @param ACC v
;;; @clobber R0 R1 R2
answer:
        sacl    vA
        pop                     ; return address
        sacl    answer_ret
        call    putspace
        lack    '='
        call    putchar
        call    putspace
        lac     vA
        call    print_int16     ; print v
        call    newline
        zals    answer_ret
        push                    ; return address
        ret

;;; Print "A rel B"
;;; @param .word A
;;; @param .word B
;;; @clobber R0 R1 R2
comp:
        pop                     ; return address
        tblr    vA              ; load A
        add     one
        tblr    vB              ; load B
        add     one
        sacl    comp_ret
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
        zals    comp_ret
        push                    ; return address
        ret

arith:
        call    expr
        .word   18000
        .word   28000
        .word   '+'
        add     vB
        call    answer         ; -19536

        call    expr
        .word   18000
        .word   -18000
        .word   '+'
        add     vB
        call    answer         ; 0

        call    expr
        .word   -18000
        .word   -18000
        .word   '+'
        add     vB
        call    answer         ; 29536

        call    expr
        .word   -18000
        .word   -28000
        .word   '-'
        sub     vB
        call    answer         ; 10000

        call    expr
        .word   18000
        .word   -18000
        .word   '-'
        sub     vB
        call    answer         ; -29536

        call    expr
        .word   -28000
        .word   -18000
        .word   '-'
        sub     vB
        call    answer         ; -10000

        call    expr
        .word   100
        .word   300
        .word   '*'
        mpy     vB
        pac
        call    answer         ; 30000

        call    expr
        .word   200
        .word   100
        .word   '*'
        mpy     vB
        pac
        call    answer         ; 20000

        call    expr
        .word   300
        .word   -200
        .word   '*'
        mpy     vB
        pac
        call    answer         ; 5536

        call    expr
        .word   100
        .word   -300
        .word   '*'
        mpy     vB
        pac
        call    answer         ; -30000

        call    expr
        .word   -200
        .word   -100
        .word   '*'
        mpy     vB
        pac
        call    answer         ; 20000

        call    expr
        .word   30000
        .word   100
        .word   '/'
        call    div16
        call    answer         ; 300

        call    expr
        .word   -200
        .word   100
        .word   '/'
        call    div16
        call    answer         ; -2

        call    expr
        .word   -30000
        .word   -200
        .word   '/'
        call    div16
        call    answer         ; 150

        call    expr
        .word   -30000
        .word   78
        .word   '/'
        call    div16
        call    answer         ; -384

        call    comp
        .word   5000
        .word   4000

        call    comp
        .word   5000
        .word   5000

        call    comp
        .word   4000
        .word   5000

        call    comp
        .word   -5000
        .word   -4000

        call    comp
        .word   -5000
        .word   -5000

        call    comp
        .word   -4000
        .word   -5000

        call    comp
        .word   32700
        .word   32600

        call    comp
        .word   32700
        .word   32700

        call    comp
        .word   32600
        .word   32700

        call    comp
        .word   -32700
        .word   -32600

        call    comp
        .word   -32700
        .word   -32700

        call    comp
        .word   -32600
        .word   -32700

        call    comp
        .word   18000
        .word   -28000

        call    comp
        .word   -28000
        .word   -28000

        call    comp
        .word   -28000
        .word   18000

        .word   HALT

        include "arith.inc"
