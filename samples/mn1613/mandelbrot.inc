*** -*- mode: asm; mode: flyspell-prog* -*-

F:      equ     50

mandelbrot:
        mvwi    R0, -12
        st      R0, vY          Y=-12
loop_y:
        mvwi    R0, -49
        st      R0, vX          X=-49
loop_x:
        l       R1, vX
        mvwi    R2, 229
        bald    mul16           R1=X*229
        mvwi    R2, 100
        bald    divu16          R1=X*229/100
        st      R1, vC          C=X*229/100
        st      R1, vA          A=C
        l       R1, vY
        mvwi    R2, 416
        bald    mul16           R1=Y*416
        mvwi    R2, 100
        bald    divu16          R1=Y*416/100
        st      R1, vD          D=Y*416/100
        st      R1, vB          B=D
        eor     R2, R2
        st      R2, vI          I=0

        * l       R1, vY
        * mvi     R0, 'Y'
        * bald    debug
        * l       R1, vX
        * mvi     R0, 'X'
        * bald    debug
        * l       R1, vC
        * mvi     R0, 'C'
        * bald    debug
        * l       R1, vD
        * mvi     R0, 'D'
        * bald    debug
        * bald    newline

loop_i:
        l       R1, vB
        mvwi    R2, F
        bald    divu16          R1=B/F
        st      R1, vQ          Q=B/F
        mvwi    R2, -F
        bald    mul16           R1=-Q*F
        l       R2, vB
        a       R1, R2          R1=B-Q*F
        st      R1, vS          S=B-Q*F
        l       R1, vB
        mv      R2, R1
        bald    mul16           R1=B*B
        mv      R3, R1          R3=B*B
        l       R1, vA
        mv      R2, R1
        bald    mul16           R1=A*A
        s       R1, R3          R1=A*A-B*B
        mvwi    R2, F
        bald    divu16          R1=(A*A-B*B)/F
        l       R2, vC
        a       R1, R2          R1=(A*A-B*B)/F+C
        mv      R3, R1          R3=(A*A-B*B)/F+C
        l       R1, vA
        l       R2, vQ
        bald    mul16           R1=A*Q
        mv      R4, R1          R4=A*Q
        l       R1, vA
        l       R2, vS
        bald    mul16           R1=A*S
        mvwi    R2, F
        bald    divu16          R1=A*S/F
        a       R1, R4          R1=A*Q+A*S/F
        a       R1, R1          R1=2*(A*Q+A*S/F)
        l       R2, vD
        a       R1, R2          R1=2*(A*Q+A*S/F)+D
        st      R1, vB          B=2*(A*Q+A*S/F)+D
        mv      R1, R3          R1=(A*A-B*B)/F+C
        st      R1, vA          A=(A*A-B*B)/F+C
        mvwi    R2, F
        bald    divu16          R1=A/F
        st      R1, vP          P=A/F
        mv      R2, R1
        bald    mul16           R1=P*P
        mv      R3, R1          R3=P*P
        l       R1, vB
        mvwi    R2, F
        bald    divu16          R1=B/F
        st      R1, vQ          Q=B/F
        mv      R2, R1
        bald    mul16           R1=Q*Q
        a       R1, R3          R1=P*P+Q*Q
        st      R1, vT          T=P*P+Q*Q

        * bald    putspace
        * l       R1, vI
        * mvi     R0, 'I'
        * bald    debug
        * l       R1, vA
        * mvi     R0, 'A'
        * bald    debug
        * l       R1, vB
        * mvi     R0, 'B'
        * bald    debug
        * l       R1, vP
        * mvi     R0, 'P'
        * bald    debug
        * l       R1, vQ
        * mvi     R0, 'Q'
        * bald    debug
        * l       R1, vT
        * mvi     R0, 'T'
        * bald    debug
        * bald    newline

        l       R0, vT
        cwi     R0, 4, mz       skip if T<=4
        b       print_i         if 4<T
        l       R0, vI
        ai      R0, 1
        st      R0, vI
        cwi     R0, 16, lpz     skip if I>=16
        b       loop_i          if I<16
        mvi     R0, ' '
        b       print_char
print_i:
        l       R0, vI
        cwi     R0, 10, lpz     skip if I>=10
        b       print_dec
        si      R0, 10
        awi     R0, 'A', skp
print_dec:
        awi     R0, '0'
print_char:
        bald    putchar

        * mv      R2, R0
        * mvi     R0, '@'
        * bald    putchar
        * mvi     R0, '='
        * bald    putchar
        * mv      R0, R2
        * bald    putchar
        * bald    newline

        bald    getchar
        or      R0, R0, enz     skip if E=1
        b       next_x
        or      R0, R0, nz      skip if R0!=0
        h                       halt to system
next_x:
        l       R0, vX
        ai      R0, 1
        st      R0, vX
        cwi     R0, 30, pz      skip if X>=30
        bd      loop_x          if X<30
next_y:
        bald    newline
        l       R0, vY
        ai      R0, 1
        st      R0, vY
        cwi     R0, 13, pz      skip if Y>=13
        bd      loop_y          if Y<13
        ret

*** Print variable: "name=variable "
*** @param R1 variable
*** @param R0 variable name
*** @clobber R0 R1
debug:
        bald    putchar     Print name
        mvi     R0, '='
        bald    putchar
        mv      R0, R1
        bald    print_int16
        bd      putspace
