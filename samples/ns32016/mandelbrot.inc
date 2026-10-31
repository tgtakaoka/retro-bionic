;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; samples/arith/mandelbrot.c in 32-bit registers: QUOD divides rounded
;;; toward zero, as C does.

Fv:     equ     50              ; fixed-point 1.0
;;; Print one 79x25 frame of the Mandelbrot set: each point is its
;;; escape iteration in hex, or a space if it does not escape in 16.
;;; Exits to the debugger when NUL is received.
;;; @clobber R0 R1 R2
mandelbrot:
        movd    -12, @vY        ; Y=-12
loop_y:
        movd    -49, @vX        ; X=-49
loop_x:
        movd    @vX, r0
        muld    229, r0
        quod    100, r0
        movd    r0, @vC         ; C=X*229/100
        movd    r0, @vA         ; A=C
        movd    @vY, r0
        muld    416, r0
        quod    100, r0
        movd    r0, @vD         ; D=Y*416/100
        movd    r0, @vB         ; B=D
        movqd   0, @vI          ; I=0

loop_i:
        movd    @vB, r0
        quod    Fv, r0
        movd    r0, @vQ         ; Q=B/F
        muld    Fv, r0
        movd    @vB, r1
        subd    r0, r1
        movd    r1, @vS         ; S=B-Q*F
        movd    @vA, r0
        muld    r0, r0          ; A*A
        movd    @vB, r1
        muld    r1, r1          ; B*B
        subd    r1, r0
        quod    Fv, r0
        addd    @vC, r0
        movd    r0, r2          ; R2=(A*A-B*B)/F+C
        movd    @vA, r0
        muld    @vS, r0
        quod    Fv, r0          ; A*S/F
        movd    @vA, r1
        muld    @vQ, r1         ; A*Q
        addd    r1, r0
        addd    r0, r0
        addd    @vD, r0
        movd    r0, @vB         ; B=2*(A*Q+A*S/F)+D
        movd    r2, @vA         ; A=(A*A-B*B)/F+C
        quod    Fv, r2          ; P=A/F
        muld    r2, r2          ; P*P
        movd    @vB, r0
        quod    Fv, r0          ; Q=B/F
        muld    r0, r0          ; Q*Q
        addd    r0, r2          ; T=P*P+Q*Q
        cmpd    r2, 4
        bgt     print_i         ; if T>4
        addqd   1, @vI          ; I+=1
        cmpd    @vI, 16
        blt     loop_i          ; if I<16
        movb    ' ', r0
        br      print_char
print_i:
        movd    @vI, r0
        cmpd    r0, 10
        blt     print_digit
        addd    'A'-'0'-10, r0
print_digit:
        addd    '0', r0
print_char:
        bsr     putchar

        bsr     getchar
        bfc     next_x
        cmpqb   0, r0
        bne     next_x
halt_to_system:
        bpt

next_x:
        addqd   1, @vX          ; X+=1
        cmpd    @vX, 30
        blt     loop_x          ; if X<30
        bsr     newline
        addqd   1, @vY          ; Y+=1
        cmpd    @vY, 13
        blt     loop_y          ; if Y<13
        ret     0

;;; workspace
vC:     .double 0
vD:     .double 0
vA:     .double 0
vB:     .double 0
vQ:     .double 0
vS:     .double 0
vY:     .double 0
vX:     .double 0
vI:     .double 0
