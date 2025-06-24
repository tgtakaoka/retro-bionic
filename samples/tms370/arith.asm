;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "tms370.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     10F0H
        include "mc6850.inc"

;;; Internal register file
Rd:     equ     5       ; R4:R5
Rs:     equ     7       ; R6:R7
vA:     equ     9       ; R8:R9
vB:     equ     11      ; R10:R11

        org     0C0H
stack:  equ     $-1      ; TMS370's SP is pre-increment/post-decrement

        org     VEC_RESET
        .word   initialize

        org     VEC_TRAP15
        .word   VEC_TRAP15

        org     2000H
initialize:
        mov     #stack, B
        ldsp
        mov     #CDS_RESET_gc, ACIA_control     ; Master reset
        mov     #WSB_8N1_gc, ACIA_control       ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled

        call    arith
        trap    15

;;; Print out char
;;; @param A char
;;; @clobber A
putspace:
        mov     #' ', A
        jmp     putchar
newline:
        mov     #00DH, A
        call    putchar
        mov     #00AH, A
putchar:
        btjz    #TDRE_bm, ACIA_status, putchar
        mov     A, ACIA_data
        rts

;;; Print "v1 op v2"
;;; @param vA v1
;;; @param vB v2
;;; @param A op letter
;;; @clobber Rd Rs
expr:
        push    A
        movw    vA, Rd
        call    print_int16    print v1
        call    putspace
        pop     A
        call    putchar        print op
        call    putspace
        movw    vB, Rd
        call    print_int16    print v2
        movw    vA, Rd
        movw    vB, Rs
        rts

;;; Print " = v\n"
;;; @param Rd v
;;; @clobber Rd Rs
answer:
        call    putspace
        mov     #'=', A
        call    putchar
        call    putspace
        call    print_int16    print v
        jmp     newline

;;; Print "v1 rel v2"
;;; @param R8:vA v1
;;; @param R10:vB v2
;;; @clobber Rd Rs
comp:
        movw    vA, Rd
        movw    vB, Rs
        call    cmp16
        tst     A
        jeq     comp_eq
        jl      comp_lt
        jg      comp_gt
        mov     #'?', A
        jmp     comp_out
comp_gt:
        mov     #'>', A
        jmp     comp_out
comp_eq:
        mov     #'=', A
        jmp     comp_out
comp_lt:
        mov     #'<', A
comp_out:
        call    expr
        jmp     newline

arith:
        movw    #18000, vA
        movw    #28000, vB
        mov     #'+', A
        call    expr
        call    add16          ; Rd=Rd+Rs
        call    answer         ; -19536

        movw    #18000, vA
        movw    #-18000, vB
        mov     #'+', A
        call    expr
        call    add16          ; Rd=Rd+Rs
        call    answer         ; 0

        movw    #-18000, vA
        movw    #-18000, vB
        mov     #'+', A
        call    expr
        call    add16          ; Rd=Rd+Rs
        call    answer         ; 29536

        movw    #-18000, vA
        movw    #-28000, vB
        mov     #'-', A
        call    expr
        call    sub16          ; Rd=Rd-Rs
        call    answer         ; 10000

        movw    #18000, vA
        movw    #-18000, vB
        mov     #'-', A
        call    expr
        call    sub16          ; Rd=Rd-Rs
        call    answer         ; 29536

        movw    #-28000, vA
        movw    #-18000, vB
        mov     #'-', A
        call    expr
        call    sub16          ; Rd=Rd-Rs
        call    answer         ; -10000

        movw    #100, vA
        movw    #300, vB
        mov     #'*', A
        call    expr
        call    mul16          ; Rd=Rd*Rs
        call    answer         ; 30000

        movw    #200, vA
        movw    #100, vB
        mov     #'*', A
        call    expr
        call    mul16          ; Rd=Rd*Rs
        call    answer         ; 20000

        movw    #300, vA
        movw    #-200, vB
        mov     #'*', A
        call    expr
        call    mul16          ; Rd=Rd*Rs
        call    answer         ; 5536

        movw    #100, vA
        movw    #-300, vB
        mov     #'*', A
        call    expr
        call    mul16          ; Rd=Rd*Rs
        call    answer         ; -30000

        movw    #-200, vA
        movw    #-100, vB
        mov     #'*', A
        call    expr
        call    mul16          ; Rd=Rd*Rs
        call    answer         ; 20000

        movw    #30000, vA
        movw    #100, vB
        mov     #'/', A
        call    expr
        call    div16          ; Rd=Rd/Rs
        call    answer         ; 300

        movw    #-200, vA
        movw    #100, vB
        mov     #'/', A
        call    expr
        call    div16          ; Rd=Rd/Rs
        call    answer         ; -2

        movw    #-30000, vA
        movw    #-200, vB
        mov     #'/', A
        call    expr
        call    div16          ; Rd=Rd/Rs
        call    answer         ; 150

        movw    #-30000, vA
        movw    #78, vB
        mov     #'/', A
        call    expr
        call    div16          ; Rd=Rd/Rs
        call    answer         ; -384

        movw    #5000, vA
        movw    #4000, vB
        call    comp

        movw    #5000, vA
        movw    #5000, vB
        call    comp

        movw    #4000, vA
        movw    #5000, vB
        call    comp

        movw    #-5000, vA
        movw    #-4000, vB
        call    comp

        movw    #-5000, vA
        movw    #-5000, vB
        call    comp

        movw    #-4000, vA
        movw    #-5000, vB
        call    comp

        movw    #32700, vA
        movw    #32600, vB
        call    comp

        movw    #32700, vA
        movw    #32700, vB
        call    comp

        movw    #32600, vA
        movw    #32700, vB
        call    comp

        movw    #-32700, vA
        movw    #-32600, vB
        call    comp

        movw    #-32700, vA
        movw    #-32700, vB
        call    comp

        movw    #-32600, vA
        movw    #-32700, vB
        call    comp

        movw    #18000, vA
        movw    #-28000, vB
        call    comp

        movw    #-28000, vA
        movw    #-28000, vB
        call    comp

        movw    #-28000, vA
        movw    #!18000, vB
        call    comp
        rts

        include "arith.inc"
