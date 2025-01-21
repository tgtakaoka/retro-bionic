*** -*- mode: asm; mode: flyspell-prog* -*-

kCA:    dc      0.0458
kCB:    dc      0.08333

fmandel:
        mvwi    R3, vA          R3=&A
        mvwi    R4, vB          R4=&B
        mvwi    R0, -12
        st      R0, vY          Y=-12
loop_y:
        mvwi    R0, -49
        st      R0, vX          X=-49
loop_x:
        l       R0, vX
        flt     DR0, R0
        mvwi    R2, kCA
        fm      DR0, (R2)       X*0.0458
        st      R0, vCA
        st      R1, vCA+1       CA=X*0.0458
        st      R0, vA
        st      R1, vA+1        A=CA
        l       R0, vY
        flt     DR0, R0
        mvwi    R2, kCB
        fm      DR0, (R2)       Y*0.08333
        st      R0, vCB
        st      R1, vCB+1       CB=Y*0.08333
        st      R0, vB
        st      R1, vB+1        B=CB
        eor     R2, R2
        st      R2, vI          I=0
loop_i:
        l       R0, vB
        l       R1, vB+1
        fm      DR0, (R4)       B*B
        st      R0, vT
        st      R1, vT+1        T=B*B
        l       R0, vA
        l       R1, vA+1
        fm      DR0, (R3)       A*A
        mvwi    R2, vT
        fs      DR0, (R2)       A*A-B*B
        mvwi    R2, vCA
        fa      DR0, (R2)       A*A-B*B+CA
        st      R0, vT
        st      R1, vT+1        T=A*A-B*B+CA
        l       R0, vA
        l       R1, vA+1
        fa      DR0, (R3)       2*A
        fm      DR0, (R4)       2*A*B
        mvwi    R2, vCB
        fa      DR0, (R2)       2*A*B+CB
        st      R0, vB
        st      R1, vB+1        B=2*A*B+CB
        l       R0, vT
        l       R1, vT+1
        st      R0, vA
        st      R1, vA+1        A=T
        fm      DR0, (R3)       A*A
        st      R0, vT
        st      R1, vT+1        T=A*A
        l       R0, vB
        l       R1, vB+1
        fm      DR0, (R4)       B*B
        mvwi    R2, vT
        fa      DR0, (R2)       A*A+B*B
        fix     R0, DR0         R0=int(A*A+B*B)
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
