;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "i8096.inc"

        org     1000H
stack:  equ     $

        org     1CH
EAX:    equ     $
AX:     equ     $
AL:     dsb     1
AH:     dsb     1
        dsw     1
EDX:    equ     $
DX:     equ     $
DL:     dsb     1
DH:     dsb     1
        dsw     1
CDX:    equ     $
CX:     equ     $
CL:     dsb     1
CH:     dsb     1
        dsw     1

        org     ORG_RESET
        nop
        nop
init:
        ld      SP, #stack
        ldb     INT_MASK, INT_EXTINT ; enable EXTINT
        ei                           ; for halt switch
        clr     AX
        not     AX
        ld      DX, #9ABCH
        nop
        nop
        nop
        nop

        skip    AL
        clr     AX
        not     AX
        neg     AX
        dec     AX
        ext     AX
        inc     AX
        shr     AX,15
        shl     AX,15
        shra    AX,15
        shrl    EAX,15
        shll    EAX,15
        shral   EAX,15
        norml   EAX,DL

        clrb    AL
        notb    AH
        negb    DL
        decb    DH
        extb    AX
        incb    DL
        shrb    DH,7
        shlb    AL,7
        shrab   AH,7

        scall    scall_a
scall_e:
        scall    scall_b
scall_b:
        scall    scall_d
scall_a:
        scall    scall_e
scall_d:

        jbc     ZERO, 0, jbc_a
jbc_b:  ldb     AH, #80H
        nop
        nop
        nop
        jbs     AH, 7, jbc_c
        nop
        nop
        nop
jbc_a:
        jbc     ZERO, 7, jbc_b
jbc_c:

        and     AX, CX, DX
        and     AX, CX, #1234H
        and     AX, CX, [DX]
        and     AX, CX, [DX]+
        and     AX, CX, 56H[DX]
        and     AX, CX, 789AH[DX]

        add     AX, CX, DX
        add     AX, CX, #1234H
        add     AX, CX, [DX]
        add     AX, CX, [DX]+
        add     AX, CX, 56H[DX]
        add     AX, CX, 789AH[DX]

        sub     AX, CX, DX
        sub     AX, CX, #1234H
        sub     AX, CX, [DX]
        sub     AX, CX, [DX]+
        sub     AX, CX, 56H[DX]
        sub     AX, CX, 789AH[DX]

        mulu    AX, CX, DX
        mulu    AX, CX, #1234H
        mulu    AX, CX, [DX]
        mulu    AX, CX, [DX]+
        mulu    AX, CX, 56H[DX]
        mulu    AX, CX, 789AH[DX]

        andb    AL, CL, DL
        andb    AL, CL, #34H
        andb    AL, CL, [DX]
        andb    AL, CL, [DX]+
        andb    AL, CL, 56H[DX]
        andb    AL, CL, 789AH[DX]

        addb    AL, CL, DL
        addb    AL, CL, #34H
        addb    AL, CL, [DX]
        addb    AL, CL, [DX]+
        addb    AL, CL, 56H[DX]
        addb    AL, CL, 789AH[DX]

        subb    AL, CL, DL
        subb    AL, CL, #34H
        subb    AL, CL, [DX]
        subb    AL, CL, [DX]+
        subb    AL, CL, 56H[DX]
        subb    AL, CL, 789AH[DX]

        mulub   AL, CL, DL
        mulub   AL, CL, #34H
        mulub   AL, CL, [DX]
        mulub   AL, CL, [DX]+
        mulub   AL, CL, 56H[DX]
        mulub   AL, CL, 789AH[DX]

        and     AX, DX
        and     AX, #1234H
        and     AX, [DX]
        and     AX, [DX]+
        and     AX, 56H[DX]
        and     AX, 789AH[DX]

        add     AX, DX
        add     AX, #1234H
        add     AX, [DX]
        add     AX, [DX]+
        add     AX, 56H[DX]
        add     AX, 789AH[DX]

        sub     AX, DX
        sub     AX, #1234H
        sub     AX, [DX]
        sub     AX, [DX]+
        sub     AX, 56H[DX]
        sub     AX, 789AH[DX]

        mulu    AX, DX
        mulu    AX, #1234H
        mulu    AX, [DX]
        mulu    AX, [DX]+
        mulu    AX, 56H[DX]
        mulu    AX, 789AH[DX]

        andb    AH, DL
        andb    AH, #34H
        andb    AH, [DX]
        andb    AH, [DX]+
        andb    AH, 56H[DX]
        andb    AH, 789AH[DX]

        addb    AH, DL
        addb    AH, #34H
        addb    AH, [DX]
        addb    AH, [DX]+
        addb    AH, 56H[DX]
        addb    AH, 789AH[DX]

        subb    AH, DL
        subb    AH, #34H
        subb    AH, [DX]
        subb    AH, [DX]+
        subb    AH, 56H[DX]
        subb    AH, 789AH[DX]

        mulub   AX, DL
        mulub   AX, #34H
        mulub   AX, [DX]
        mulub   AX, [DX]+
        mulub   AX, 56H[DX]
        mulub   AX, 789AH[DX]

        or      AX, DX
        or      AX, #1234H
        or      AX, [DX]
        or      AX, [DX]+
        or      AX, 56H[DX]
        or      AX, 789AH[DX]

        xor     AX, DX
        xor     AX, #1234H
        xor     AX, [DX]
        xor     AX, [DX]+
        xor     AX, 56H[DX]
        xor     AX, 789AH[DX]

        cmp     AX, DX
        cmp     AX, #1234H
        cmp     AX, [DX]
        cmp     AX, [DX]+
        cmp     AX, 56H[DX]
        cmp     AX, 789AH[DX]

        divu    EAX, DX
        divu    EAX, #1234H
        divu    EAX, [DX]
        divu    EAX, [DX]+
        divu    EAX, 56H[DX]
        divu    EAX, 789AH[DX]

        orb     AH, DL
        orb     AH, #34H
        orb     AH, [DX]
        orb     AH, [DX]+
        orb     AH, 56H[DX]
        orb     AH, 789AH[DX]

        xorb    AH, DL
        xorb    AH, #34H
        xorb    AH, [DX]
        xorb    AH, [DX]+
        xorb    AH, 56H[DX]
        xorb    AH, 789AH[DX]

        cmpb    AH, DL
        cmpb    AH, #34H
        cmpb    AH, [DX]
        cmpb    AH, [DX]+
        cmpb    AH, 56H[DX]
        cmpb    AH, 789AH[DX]

        divub   AX, DL
        divub   AX, #34H
        divub   AX, [DX]
        divub   AX, [DX]+
        divub   AX, 56H[DX]
        divub   AX, 789AH[DX]

        ld      AX, DX
        ld      AX, #1234H
        ld      AX, [DX]
        ld      AX, [DX]+
        ld      AX, 56H[DX]
        ld      AX, 789AH[DX]

        addc    AX, DX
        addc    AX, #1234H
        addc    AX, [DX]
        addc    AX, [DX]+
        addc    AX, 56H[DX]
        addc    AX, 789AH[DX]

        subc    AX, DX
        subc    AX, #1234H
        subc    AX, [DX]
        subc    AX, [DX]+
        subc    AX, 56H[DX]
        subc    AX, 789AH[DX]

        ldbze   AX, DL
        ldbze   AX, #34H
        ldbze   AX, [DX]
        ldbze   AX, [DX]+
        ldbze   AX, 56H[DX]
        ldbze   AX, 789AH[DX]

        ldb     AH, DL
        ldb     AH, #34H
        ldb     AH, [DX]
        ldb     AH, [DX]+
        ldb     AH, 56H[DX]
        ldb     AH, 789AH[DX]

        addcb   AH, DL
        addcb   AH, #34H
        addcb   AH, [DX]
        addcb   AH, [DX]+
        addcb   AH, 56H[DX]
        addcb   AH, 789AH[DX]

        subcb   AH, DL
        subcb   AH, #34H
        subcb   AH, [DX]
        subcb   AH, [DX]+
        subcb   AH, 56H[DX]
        subcb   AH, 789AH[DX]

        ldbse   AX, DL
        ldbse   AX, #34H
        ldbse   AX, [DX]
        ldbse   AX, [DX]+
        ldbse   AX, 56H[DX]
        ldbse   AX, 789AH[DX]

        st      AX, CX
        st      AX, [DX]
        st      AX, [DX]+
        st      AX, 56H[DX]
        st      AX, 789AH[DX]

        stb     AH, CL
        stb     AH, [DX]
        stb     AH, [DX]+
        stb     AH, 56H[DX]
        stb     AH, 789AH[DX]

        push    DX
        push    #1234H
        push    [DX]
        push    [DX]+
        push    56H[DX]
        push    789AH[DX]

        pop     CX
        pop     [DX]
        pop     [DX]+
        pop     56H[DX]
        pop     789AH[DX]

        clrc
        jc      jcc_a
        nop
        nop
        nop
        nop
        nop
        jnc     jcc_b
        nop
        nop
        nop
        nop
        nop
jcc_b:
        nop
        nop
        nop
        nop
        nop
jcc_a:

        ldb     AH, #2
djnz_a:
        djnz    AH, djnz_a
        nop
        nop
        nop
        nop
        nop
        ld      DX, #br_a
        br      [DX]
        nop
        nop
        nop
        nop
        nop
br_a:

        ljmp    ljmp_a
        nop
        nop
        nop
        nop
        nop
        nop
ljmp_a:

        lcall   lcall_b
        ljmp    lcall_a
        nop
        nop
        nop
        nop
        nop
        nop
lcall_b:
        ret
lcall_a:

        pushf
        popf
        clrc
        setc
        di
        ei
        clrvt
        ld      AX, #20E3H      ; br [DX]
        st      AX, ORG_RESET
        ld      DX, #rst_a
        rst
rst_a:

        ld      AX, #trap_a
        st      AX, VEC_TRAP
        trap
        nop
        nop
        nop
        nop
        nop
        nop
trap_a:

        mul     EAX, CX, DX
        mul     EAX, CX, #1234H
        mul     EAX, CX, [DX]
        mul     EAX, CX, [DX]+
        mul     EAX, CX, 56H[DX]
        mul     EAX, CX, 789AH[DX]

        mulb    AX, CL, DL
        mulb    AX, CL, #34H
        mulb    AX, CL, [DX]
        mulb    AX, CL, [DX]+
        mulb    AX, CL, 56H[DX]
        mulb    AX, CL, 789AH[DX]

        mul     EAX, DX
        mul     EAX, #1234H
        mul     EAX, [DX]
        mul     EAX, [DX]+
        mul     EAX, 56H[DX]
        mul     EAX, 789AH[DX]

        mulb    AX, DL
        mulb    AX, #34H
        mulb    AX, [DX]
        mulb    AX, [DX]+
        mulb    AX, 56H[DX]
        mulb    AX, 789AH[DX]

        div     EAX, DX
        div     EAX, #1234H
        div     EAX, [DX]
        div     EAX, [DX]+
        div     EAX, 56H[DX]
        div     EAX, 789AH[DX]

        divb    AX, DL
        divb    AX, #34H
        divb    AX, [DX]
        divb    AX, [DX]+
        divb    AX, 56H[DX]
        divb    AX, 789AH[DX]

        nop
        nop
        ld      AX, #0FDFDH     ; nop
        st      AX, ORG_RESET
        nop
        nop
        ld      AX, #VEC_TRAP
        st      AX, VEC_TRAP
        trap
        nop
        nop
        nop
        nop
