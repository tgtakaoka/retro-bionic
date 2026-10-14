*** -*- mode: asm; mode: flyspell-prog* -*-

F:      equ     50

*** M and D are unsigned and read their operand through (Ri), so the
*** loop works on |A| and |B| in vA and vB, keeps R2 pointing at vB
*** and R3 at the constant F, and keeps the sign of A*B in R4. A and B
*** stay small while a point is iterated, so every product fits in 16
*** bits and M leaves R0=0, ready for D.

mandelbrot:
        mvwi    R0, -12
        st      R0, vY          Y=-12
loop_y:
        l       R0, vY
        mv      R4, R0          R4=sign(Y)
        tbit    R0, 0, z        skip if Y>=0
        neg     R0, c           R0=|Y|
        mvwi    R2, C416
        m       DR0, (R2)       DR0=|Y|*416
        mvwi    R3, C100
        d       DR0, (R3)       R0=|Y|*416/100
        st      R0, vE          E=|D|
        tbit    R4, 0, z        skip if Y>=0
        neg     R0, c
        st      R0, vD          D=Y*416/100
        mvwi    R0, -49
        st      R0, vX          X=-49
loop_x:
        l       R0, vX
        mv      R4, R0          R4=sign(X)
        tbit    R0, 0, z        skip if X>=0
        neg     R0, c           R0=|X|
        mvwi    R2, C229
        m       DR0, (R2)       DR0=|X|*229
        mvwi    R3, C100
        d       DR0, (R3)       R0=|X|*229/100
        st      R0, vA          A=|C|
        tbit    R4, 0, z        skip if X>=0
        neg     R0, c
        st      R0, vC          C=X*229/100
        l       R1, vD
        mv      R4, R0
        eor     R4, R1          R4=sign(C*D)
        l       R0, vE
        st      R0, vB          B=|D|
        mvwi    R2, vB          R2 points B
        mvwi    R3, CF          R3 points F
        mvwi    R0, -16
        st      R0, vI          I=0, kept as I-16

        * push    R2              debug clobbers R2
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
        * pop     R2

loop_i:
        l       R0, vA
        m       DR0, (R2)       DR0=|A|*|B|
        d       DR0, (R3)       R0=|A*B|/F
        tbit    R4, 0, z        skip if A*B>=0
        neg     R0, c           R0=A*B/F
        a       R0, R0
        l       R1, vD
        a       R0, R1          R0=2*A*B/F+D
        st      R0, vN          N=2*A*B/F+D, the next B
        l       R0, vA
        l       R1, vB
        a       R1, R0          R1=|A|+|B|
        str     R1, (R2)        B=|A|+|B| for M
        a       R0, R0
        s       R0, R1          R0=|A|-|B|
        mv      R4, R0          R4=sign(A*A-B*B)
        tbit    R0, 0, z        skip if |A|>=|B|
        neg     R0, c           R0=||A|-|B||
        m       DR0, (R2)       DR0=|(|A|+|B|)*(|A|-|B|)|=|A*A-B*B|
        d       DR0, (R3)       R0=|A*A-B*B|/F
        tbit    R4, 0, z        skip if A*A>=B*B
        neg     R0, c           R0=(A*A-B*B)/F
        l       R1, vC
        a       R0, R1          R0=(A*A-B*B)/F+C, the next A
        l       R1, vN          R1=next B
        mv      R4, R0
        eor     R4, R1          R4=sign(A*B)
        tbit    R1, 0, z        skip if B>=0
        neg     R1, c
        st      R1, vB          B=|B|
        tbit    R0, 0, z        skip if A>=0
        neg     R0, c
        st      R0, vA          A=|A|
        ai      R3, 1           R3 points 1/F
        m       DR0, (R3)       R0=|A|*RF/65536=|A|/F=|P|
        si      R2, 1           R2 points W
        str     R0, (R2)        W=|P|
        mv      R1, R0, z       skip if P=0, then P*P=R1=0
        m       DR0, (R2)       R1=P*P
        st      R1, vT          T=P*P
        l       R0, vB
        m       DR0, (R3)       R0=|B|/F=|Q|
        str     R0, (R2)        W=|Q|
        mv      R1, R0, z       skip if Q=0, then Q*Q=R1=0
        m       DR0, (R2)       R1=Q*Q
        ai      R2, 1           R2 points B
        si      R3, 1           R3 points F
        l       R0, vT
        a       R1, R0          R1=T=P*P+Q*Q

        * st      R1, vT
        * push    R2              debug clobbers R2
        * bald    putspace
        * l       R1, vI
        * awi     R1, 16
        * mvi     R0, 'I'
        * bald    debug
        * l       R1, vA
        * mvi     R0, 'A'
        * bald    debug
        * l       R1, vB
        * mvi     R0, 'B'
        * bald    debug
        * l       R1, vT
        * mvi     R0, 'T'
        * bald    debug
        * bald    newline
        * pop     R2
        * l       R1, vT

        si      R1, 5, m        skip if T<=4
        b       print_i         if 4<T
        ims     vI              I+=1, skip if I>=16
        b       loop_i          if I<16
        mvi     R0, ' '
        b       print_char
print_i:
        l       R0, vI
        awi     R0, 16          R0=I
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

        mvwi    X0, rx_queue
        l       R0, queue_len(X0)
        or      R0, R0, nz      skip if a character is received
        b       next_x
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

CF:     dc      F
*** |A|/F and |B|/F take the high word of a product with RF: exact
*** below 65536/(RF*F-65536)=4681, and |A| and |B| stay below 1000.
RF:     equ     (65536+F-1)/F
CRF:    dc      RF              must follow CF
C100:   dc      100
C229:   dc      229
C416:   dc      416

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
