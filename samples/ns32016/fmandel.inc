;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; samples/arith/fmandel.cpp in the NS32081's single floats.

;;; Print the Mandelbrot set for Y=-12..12, X=-49..29 in single float;
;;; each point shows its escape iteration 0-F, or a space if it never
;;; escapes. A NUL received halts the CPU.
;;; @clobber R0 F0 F1 F2
fmandel:
        movd    -12, @vY        ; Y=-12
loop_y:
        movd    -49, @vX        ; X=-49
loop_x:
        movdf   @vX, f0
        mulf    @kCA, f0
        movf    f0, @vCA        ; CA=X*0.0458
        movf    f0, @vA         ; A=CA
        movdf   @vY, f0
        mulf    @kCB, f0
        movf    f0, @vCB        ; CB=Y*0.08333
        movf    f0, @vB         ; B=CB
        movqd   0, @vI          ; I=0

loop_i:
        movf    @vB, f1
        mulf    f1, f1          ; B*B
        movf    @vA, f2
        mulf    f2, f2          ; A*A
        subf    f1, f2          ; A*A-B*B
        addf    @vCA, f2        ; T=A*A-B*B+CA
        movf    @vA, f0
        addf    f0, f0          ; 2*A
        mulf    @vB, f0         ; 2*A*B
        addf    @vCB, f0        ; 2*A*B+CB
        movf    f0, @vB         ; B=2*A*B+CB
        movf    f2, @vA         ; A=T
        mulf    f2, f2          ; A*A
        mulf    f0, f0          ; B*B
        addf    f0, f2          ; T=A*A+B*B
        cmpf    f2, @k4
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

kCA:    .float  0.0458
kCB:    .float  0.08333
k4:     .float  4.0

;;; workspace
vCA:    .float  0
vCB:    .float  0
vA:     .float  0
vB:     .float  0
vY:     .double 0
vX:     .double 0
vI:     .double 0
