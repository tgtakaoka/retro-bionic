*** -*- mode: asm; mode: flyspell-prog* -*-

F:      equ     50

mandelbrot:
        mvi     R0, hi(-12)
        bswp    R0, R0
        mvi     R0, lo(-12)
        st      R0, vY          Y=-12
loop_y:
        mvi     R0, hi(-49)
        bswp    R0, R0
        mvi     R0, lo(-49)
        st      R0, vX          X=-49
loop_x:
        l       R1, vX
        eor     R2, R2
        mvi     R2, 229
        bal     (I_mul16)       R1=X*229
        eor     R2, R2
        mvi     R2, 100
        bal     (I_divu16)      R1=X*229/100
        st      R1, vC          C=X*229/100
        st      R1, vA          A=C
        l       R2, vY          R2=Y, the shorter multiplier
        mvi     R1, hi(416)
        bswp    R1, R1
        mvi     R1, lo(416)
        bal     (I_mul16)       R1=416*Y
        eor     R2, R2
        mvi     R2, 100
        bal     (I_divu16)      R1=416*Y/100
        st      R1, vD          D=416*Y/100
        st      R1, vB          B=D
        mv      R4, R1          R4=B
        mvi     R2, hi(-16)
        bswp    R2, R2
        mvi     R2, lo(-16)
        st      R2, vI          I=0, kept as I-16

        * l       R1, vY
        * mvi     R0, 'Y'
        * bal     (I_debug)
        * l       R1, vX
        * mvi     R0, 'X'
        * bal     (I_debug)
        * l       R1, vC
        * mvi     R0, 'C'
        * bal     (I_debug)
        * l       R1, vD
        * mvi     R0, 'D'
        * bal     (I_debug)
        * bal     (I_newline)

loop_i:
        mv      R1, R4          R1=B
        eor     R2, R2
        mvi     R2, F
        bal     (I_divu16)      R1=B/F, R2=F
        st      R1, vQ          Q=B/F
        mv      R1, R1, z       skip if Q=0, then Q*F=R1=0
        bal     (I_mul16)       R1=Q*F
        mv      R2, R4
        s       R2, R1          R2=B-Q*F
        st      R2, vS          S=B-Q*F
        mv      R1, R4
        mv      R2, R1, z       skip if B=0, then B*B=R1=0
        bal     (I_mul16)       R1=B*B
        mv      R3, R1          R3=B*B
        l       R1, vA
        mv      R2, R1, z       skip if A=0, then A*A=R1=0
        bal     (I_mul16)       R1=A*A
        s       R1, R3          R1=A*A-B*B
        eor     R2, R2
        mvi     R2, F
        bal     (I_divu16)      R1=(A*A-B*B)/F
        l       R2, vC
        a       R1, R2          R1=(A*A-B*B)/F+C
        mv      R3, R1          R3=(A*A-B*B)/F+C
        l       R2, vA
        l       R1, vQ
        mv      R1, R1, z       skip if Q=0, then A*Q=R1=0
        bal     (I_mul16)       R1=A*Q
        mv      R4, R1          R4=A*Q
        l       R1, vA
        l       R2, vS
        bal     (I_mul16)       R1=A*S
        eor     R2, R2
        mvi     R2, F
        bal     (I_divu16)      R1=A*S/F, R2=F
        a       R1, R4          R1=A*Q+A*S/F
        a       R1, R1          R1=2*(A*Q+A*S/F)
        l       R0, vD
        a       R1, R0          R1=2*(A*Q+A*S/F)+D
        st      R1, vB          B=2*(A*Q+A*S/F)+D
        mv      R4, R1          R4=B
        st      R3, vA          A=(A*A-B*B)/F+C
        mv      R1, R3
        bal     (I_divu16)      R1=A/F
        st      R1, vP          P=A/F
        mv      R2, R1, z       skip if P=0, then P*P=R1=0
        bal     (I_mul16)       R1=P*P
        mv      R3, R1          R3=P*P
        mv      R1, R4
        eor     R2, R2
        mvi     R2, F
        bal     (I_divu16)      R1=B/F
        st      R1, vQ          Q=B/F
        mv      R2, R1, z       skip if Q=0, then Q*Q=R1=0
        bal     (I_mul16)       R1=Q*Q
        a       R1, R3          R1=P*P+Q*Q
        st      R1, vT          T=P*P+Q*Q

        * bal     (I_putspace)
        * l       R1, vI
        * ai      R1, 15
        * ai      R1, 1
        * mvi     R0, 'I'
        * bal     (I_debug)
        * l       R1, vA
        * mvi     R0, 'A'
        * bal     (I_debug)
        * l       R1, vB
        * mvi     R0, 'B'
        * bal     (I_debug)
        * l       R1, vP
        * mvi     R0, 'P'
        * bal     (I_debug)
        * l       R1, vQ
        * mvi     R0, 'Q'
        * bal     (I_debug)
        * l       R1, vT
        * mvi     R0, 'T'
        * bal     (I_debug)
        * bal     (I_newline)
        * l       R1, vT

        si      R1, 5, m        skip if T<=4
        b       print_i         if 4<T
        ims     vI              I+=1, skip if I>=16
        b       loop_i          if I<16
        mvi     R0, ' '
        b       print_char
print_i:
        l       R0, vI
        ai      R0, 15
        ai      R0, 1           R0=I
        mvi     R1, 10
        cb      R0, R1, lpz     skip if I>=10
        b       print_dec
        mvi     R1, 'A'
        si      R0, 10, skp
print_dec:
        mvi     R1, '0'
        a       R0, R1
print_char:
        bal     (I_putchar)

        * mv      R2, R0
        * mvi     R0, '@'
        * bal     (I_putchar)
        * mvi     R0, '='
        * bal     (I_putchar)
        * mv      R0, R2
        * bal     (I_putchar)
        * bal     (I_newline)

        bal     (I_getchar)
        or      R0, R0, enz     skip if E=1
        b       next_x
        or      R0, R0, nz      skip if R0!=0
        h                       halt to system
next_x:
        l       R0, vX
        ai      R0, 1
        st      R0, vX
        eor     R2, R2
        mvi     R2, 30
        c       R0, R2, pz      skip if X>=30
        b       (I_loop_x)      if X<30
next_y:
        bal     (I_newline)
        l       R0, vY
        ai      R0, 1
        st      R0, vY
        eor     R2, R2
        mvi     R2, 13
        c       R0, R2, pz      skip if Y>=13
        b       (I_loop_y)      if Y<13
        ret

I_loop_x:       dc      loop_x
I_loop_y:       dc      loop_y

*** Print variable: "name=variable "
*** @param R1 variable
*** @param R0 variable name
*** @clobber R0 R1
debug:
        bal     (I_putchar)     Print name
        mvi     R0, '='
        bal     (I_putchar)
        mv      R0, R1
        bal     (I_print_int16)
        b       (I_putspace)
