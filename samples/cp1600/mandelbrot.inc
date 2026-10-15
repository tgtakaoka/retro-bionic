;;; -*- mode: asm; mode: flyspell-prog; -*-

F:      equ     50              ; fixed-point scale, 1.0=F

mandelbrot:
        pshr    r5
        mvii    -12, r0
        mvo     r0, vY          ; Y=-12
loop_y:
        mvii    -49, r0
        mvo     r0, vX          ; X=-49
loop_x:
        mvi     vX, r1
        mvii    229, r2
        jsr     r5, mul16       ; R1=X*229
        mvii    100, r2
        jsr     r5, div16       ; R1=X*229/100
        mvo     r1, vC          ; C=X*229/100
        mvo     r1, vA          ; A=C
        mvi     vY, r1
        mvii    416, r2
        jsr     r5, mul16       ; R1=Y*416
        mvii    100, r2
        jsr     r5, div16       ; R1=Y*416/100
        mvo     r1, vD          ; D=Y*416/100
        mvo     r1, vB          ; B=D
        clrr    r0
        mvo     r0, vI          ; I=0

        ;; mvi     vY, r1
        ;; mvii    'Y', r0
        ;; jsr     r5, debug
        ;; mvi     vX, r1
        ;; mvii    'X', r0
        ;; jsr     r5, debug
        ;; mvi     vC, r1
        ;; mvii    'C', r0
        ;; jsr     r5, debug
        ;; mvi     vD, r1
        ;; mvii    'D', r0
        ;; jsr     r5, debug
        ;; jsr     r5, newline

loop_i:
        ;; A*B/F = A*Q+A*S/F with Q=B/F, S=B-Q*F; keeps products in 16 bits
        mvi     vB, r1
        mvii    F, r2
        jsr     r5, div16       ; R1=B/F
        mvo     r1, vQ          ; Q=B/F
        mvii    -F, r2
        jsr     r5, mul16       ; R1=-Q*F
        add     vB, r1          ; R1=B-Q*F
        mvo     r1, vS          ; S=B-Q*F
        mvi     vB, r1
        movr    r1, r2
        jsr     r5, mul16       ; R1=B*B
        movr    r1, r3          ; R3=B*B
        mvi     vA, r1
        movr    r1, r2
        jsr     r5, mul16       ; R1=A*A
        subr    r3, r1          ; R1=A*A-B*B
        mvii    F, r2
        jsr     r5, div16       ; R1=(A*A-B*B)/F
        add     vC, r1          ; R1=(A*A-B*B)/F+C
        movr    r1, r3          ; R3=(A*A-B*B)/F+C
        mvi     vA, r1
        mvi     vQ, r2
        jsr     r5, mul16       ; R1=A*Q
        movr    r1, r4          ; R4=A*Q
        mvi     vA, r1
        mvi     vS, r2
        jsr     r5, mul16       ; R1=A*S
        mvii    F, r2
        jsr     r5, div16       ; R1=A*S/F
        addr    r4, r1          ; R1=A*Q+A*S/F
        addr    r1, r1          ; R1=2*(A*Q+A*S/F)
        add     vD, r1          ; R1=2*(A*Q+A*S/F)+D
        mvo     r1, vB          ; B=2*(A*Q+A*S/F)+D
        mvo     r3, vA          ; A=(A*A-B*B)/F+C
        movr    r3, r1
        mvii    F, r2
        jsr     r5, div16       ; R1=A/F
        mvo     r1, vP          ; P=A/F
        movr    r1, r2
        jsr     r5, mul16       ; R1=P*P
        movr    r1, r3          ; R3=P*P
        mvi     vB, r1
        mvii    F, r2
        jsr     r5, div16       ; R1=B/F
        mvo     r1, vQ          ; Q=B/F
        movr    r1, r2
        jsr     r5, mul16       ; R1=Q*Q
        addr    r3, r1          ; R1=P*P+Q*Q
        mvo     r1, vT          ; T=P*P+Q*Q

        ;; jsr     r5, putspace
        ;; mvi     vI, r1
        ;; mvii    'I', r0
        ;; jsr     r5, debug
        ;; mvi     vA, r1
        ;; mvii    'A', r0
        ;; jsr     r5, debug
        ;; mvi     vB, r1
        ;; mvii    'B', r0
        ;; jsr     r5, debug
        ;; mvi     vP, r1
        ;; mvii    'P', r0
        ;; jsr     r5, debug
        ;; mvi     vQ, r1
        ;; mvii    'Q', r0
        ;; jsr     r5, debug
        ;; mvi     vT, r1
        ;; mvii    'T', r0
        ;; jsr     r5, debug
        ;; jsr     r5, newline

        mvi     vT, r0
        cmpi    4, r0
        bgt     print_i         ; branch if 4<T
        mvi     vI, r0
        incr    r0
        mvo     r0, vI          ; I+=1
        cmpi    16, r0
        blt     loop_i          ; branch if I<16
        mvii    ' ', r0
        b       print_char
print_i:
        mvi     vI, r0
        cmpi    10, r0
        blt     print_dec       ; branch if I<10
        addi    'A'-10, r0
        b       print_char
print_dec:
        addi    '0', r0
print_char:
        jsr     r5, putchar

        ;; movr    r0, r2
        ;; mvii    '@', r0
        ;; jsr     r5, putchar
        ;; mvii    '=', r0
        ;; jsr     r5, putchar
        ;; movr    r2, r0
        ;; jsr     r5, putchar
        ;; jsr     r5, newline

        jsr     r5, getchar
        bnc     next_x          ; branch if no input
        tstr    r0
        bneq    next_x
        hlt                     ; halt to system on NUL
next_x:
        mvi     vX, r0
        incr    r0
        mvo     r0, vX          ; X+=1
        cmpi    30, r0
        blt     loop_x          ; branch if X<30
        jsr     r5, newline
        mvi     vY, r0
        incr    r0
        mvo     r0, vY          ; Y+=1
        cmpi    13, r0
        blt     loop_y          ; branch if Y<13
        pulr    r7

;;; Print variable: "name=variable "
;;; @param R1 variable
;;; @param R0 variable name
;;; @clobber R0 R1 R2
debug:
        pshr    r5
        jsr     r5, putchar     ; print name
        mvii    '=', r0
        jsr     r5, putchar
        movr    r1, r0
        jsr     r5, print_int16
        pulr    r5
        b       putspace
