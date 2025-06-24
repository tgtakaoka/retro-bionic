;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tms7000
        include "tms7000.inc"

        *** MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     >01F0
        include "mc6850.inc"

        *** Internal register file
Rd:     equ     5       R4:R5
Rs:     equ     7       R6:R7
vA:     equ     9       R8:R9
vB:     equ     11      R10:R11

        org     >60
        * TMS7000's SP is pre-increment/post-decrement
stack:

        org     VEC_RESET
        data    initialize

        org     >1000
initialize:
        mov     %stack, B
        ldsp
        movp    %CDS_RESET_gc, ACIA_control     Master reset
        movp    %WSB_8N1_gc, ACIA_control       8 bits + No Parity + 1 Stop Bits
        *                                       Transmit, Receive interrupts disabled

        call    @arith
        idle

        *** Print out char
        *** @param A char
        *** @clobber A
putspace:
        mov     %' ', A
        jmp     putchar
newline:
        mov     %>0D, A
        call    @putchar
        mov     %>0A, A
putchar:
        btjzp   %TDRE_bm, ACIA_status, putchar
        movp    A, ACIA_data
        rets

        *** Print "v1 op v2"
        *** @param vA v1
        *** @param vB v2
        *** @param A op letter
        *** @clobber Rd Rs
expr:
        push    A
        movd    vA, Rd
        call    @print_int16    print v1
        call    @putspace
        pop     A
        call    @putchar        print op
        call    @putspace
        movd    vB, Rd
        call    @print_int16    print v2
        movd    vA, Rd
        movd    vB, Rs
        rets

        *** Print " = v\n"
        *** @param Rd v
        *** @clobber Rd Rs
answer:
        call    @putspace
        mov     %'=', A
        call    @putchar
        call    @putspace
        call    @print_int16    print v
        jmp     newline

        *** Print "v1 rel v2"
        *** @param R8:vA v1
        *** @param R10:vB v2
        *** @clobber Rd Rs
comp:
        movd    vA, Rd
        movd    vB, Rs
        call    @cmp16
        tsta
        jeq     comp_eq
        jlt     comp_lt
        jgt     comp_gt
        mov     %'?', A
        jmp     comp_out
comp_gt:
        mov     %'>', A
        jmp     comp_out
comp_eq:
        mov     %'=', A
        jmp     comp_out
comp_lt:
        mov     %'<', A
comp_out:
        call    @expr
        jmp     newline

arith:
        movd    %18000, vA
        movd    %28000, vB
        mov     %'+', A
        call    @expr
        call    @add16          ; Rd=Rd+Rs
        call    @answer         ; -19536

        movd    %18000, vA
        movd    %-18000, vB
        mov     %'+', A
        call    @expr
        call    @add16          ; Rd=Rd+Rs
        call    @answer         ; 0

        movd    %-18000, vA
        movd    %-18000, vB
        mov     %'+', A
        call    @expr
        call    @add16          ; Rd=Rd+Rs
        call    @answer         ; 29536

        movd    %-18000, vA
        movd    %-28000, vB
        mov     %'-', A
        call    @expr
        call    @sub16          ; Rd=Rd-Rs
        call    @answer         ; 10000

        movd    %18000, vA
        movd    %-18000, vB
        mov     %'-', A
        call    @expr
        call    @sub16          ; Rd=Rd-Rs
        call    @answer         ; 29536

        movd    %-28000, vA
        movd    %-18000, vB
        mov     %'-', A
        call    @expr
        call    @sub16          ; Rd=Rd-Rs
        call    @answer         ; -10000

        movd    %100, vA
        movd    %300, vB
        mov     %'*', A
        call    @expr
        call    @mul16          ; Rd=Rd*Rs
        call    @answer         ; 30000

        movd    %200, vA
        movd    %100, vB
        mov     %'*', A
        call    @expr
        call    @mul16          ; Rd=Rd*Rs
        call    @answer         ; 20000

        movd    %300, vA
        movd    %-200, vB
        mov     %'*', A
        call    @expr
        call    @mul16          ; Rd=Rd*Rs
        call    @answer         ; 5536

        movd    %100, vA
        movd    %-300, vB
        mov     %'*', A
        call    @expr
        call    @mul16          ; Rd=Rd*Rs
        call    @answer         ; -30000

        movd    %-200, vA
        movd    %-100, vB
        mov     %'*', A
        call    @expr
        call    @mul16          ; Rd=Rd*Rs
        call    @answer         ; 20000

        movd    %30000, vA
        movd    %100, vB
        mov     %'/', A
        call    @expr
        call    @div16          ; Rd=Rd/Rs
        call    @answer         ; 300

        movd    %-200, vA
        movd    %100, vB
        mov     %'/', A
        call    @expr
        call    @div16          ; Rd=Rd/Rs
        call    @answer         ; -2

        movd    %-30000, vA
        movd    %-200, vB
        mov     %'/', A
        call    @expr
        call    @div16          ; Rd=Rd/Rs
        call    @answer         ; 150

        movd    %-30000, vA
        movd    %78, vB
        mov     %'/', A
        call    @expr
        call    @div16          ; Rd=Rd/Rs
        call    @answer         ; -384

        movd    %5000, vA
        movd    %4000, vB
        call    @comp

        movd    %5000, vA
        movd    %5000, vB
        call    @comp

        movd    %4000, vA
        movd    %5000, vB
        call    @comp

        movd    %-5000, vA
        movd    %-4000, vB
        call    @comp

        movd    %-5000, vA
        movd    %-5000, vB
        call    @comp

        movd    %-4000, vA
        movd    %-5000, vB
        call    @comp

        movd    %32700, vA
        movd    %32600, vB
        call    @comp

        movd    %32700, vA
        movd    %32700, vB
        call    @comp

        movd    %32600, vA
        movd    %32700, vB
        call    @comp

        movd    %-32700, vA
        movd    %-32600, vB
        call    @comp

        movd    %-32700, vA
        movd    %-32700, vB
        call    @comp

        movd    %-32600, vA
        movd    %-32700, vB
        call    @comp

        movd    %18000, vA
        movd    %-28000, vB
        call    @comp

        movd    %-28000, vA
        movd    %-28000, vB
        call    @comp

        movd    %-28000, vA
        movd    %18000, vB
        call    @comp
        rets

        include "arith.inc"
