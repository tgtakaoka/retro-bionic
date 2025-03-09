*** -*- mode: asm; mode: flyspell-prog* -*-

        *** Print variable: "name=variable "
        *** @param R1 variable
        *** @param R0 variable name
        *** @clobber R0 R1
print:
        blsk    R10, putchar        Print name
        li      R0, hibyte('=')
        blsk    R10, putchar
        mov     R1, R0
        blsk    R10, print_int16
        blsk    R10, putspace
        bind    *R10+           Pop return address

F:      equ     50
vC:     bss     2
vD:     bss     2
vA:     bss     2
vB:     bss     2
vS:     bss     2
vP:     bss     2
vQ:     bss     2
vT:     bss     2
vY:     bss     2
vX:     bss     2
vI:     bss     2

mandelbrot:
        li      R0, -12
        mov     R0, @vY         Y=-12
loop_y:
        li      R0, -49
        mov     R0, @vX         X=-49
loop_x:
        mov     @vX, R1
        li      R0, 229
        mpys    R1              R1=X*229
        li      R2, 100
        bl      @divu16         R1=X*229/100
        mov     R1, @vC         C=X*229/100
        mov     R1, @vA         A=C
        mov     @vY, R1
        li      R0, 416
        mpys    R1              R1=Y*416
        li      R2, 100
        bl      @divu16         R1=Y*416/100
        mov     R1, @vD         D=Y*416/100
        mov     R1, @vB         B=D
        clr     @vI             I=0

        * mov     @vY, R1
        * li      R0, hibyte('Y')
        * blsk    R10, print
        * mov     @vX, R1
        * li      R0, hibyte('X')
        * blsk    R10, print
        * mov     @vC, R1
        * li      R0, hibyte('C')
        * blsk    R10, print
        * mov     @vD, R1
        * li      R0, hibyte('D')
        * blsk    R10, print
        * blsk    R10, newline

loop_i:
        mov     @vB, R1
        li      R2, F
        bl      @divu16         R1=B/F
        mov     R1, @vQ         Q=B/F
        li      R0, -F
        mpys    R1              R1=-Q*F
        a       @vB, R1         R1=B-Q*F
        mov     R1, @vS         S=B-Q*F
        mov     @vB, R1
        mov     R1, R0
        mpys    R1              R1=B*B
        mov     R1, R3          R3=B*B
        mov     @vA, R1
        mov     R1, R0
        mpys    R1              R1=A*A
        s       R3, R1          R1=A*A-B*B
        li      R2, F
        bl      @divu16         R1=(A*A-B*B)/F
        a       @vC, R1         R1=(A*A-B*B)/F+C
        mov     R1, R3          R3=(A*A-B*B)/F+C
        mov     @vA, R1
        mov     @vQ, R0
        mpys    R1              R1=A*Q
        mov     R1, R4          R4=A*Q
        mov     @vA, R1
        mov     @vS, R0
        mpys    R1              R1=A*S
        li      R2, F
        bl      @divu16         R1=A*S/F
        a       R4, R1          R1=A*Q+A*S/F
        a       R1, R1          R1=2*(A*Q+A*S/F)
        a       @vD, R1         R1=2*(A*Q+A*S/F)+D
        mov     R1, @vB         B=2*(A*Q+A*S/F)+D
        mov     R3, R1          R1=(A*A-B*B)/F+C
        mov     R1, @vA         A=(A*A-B*B)/F+C
        li      R2, F
        bl      @divu16         R1=A/F
        mov     R1, @vP         P=A/F
        mov     R1, R0
        mpys    R1              R1=P*P
        mov     R1, R3          R3=P*P
        mov     @vB, R1
        li      R2, F
        bl      @divu16         R1=B/F
        mov     R1, @vQ         Q=B/F
        mov     R1, R0
        mpys                    R1=Q*Q
        a       R3, R1          R1=P*P+Q*Q
        mov     R1, @vT         T=P*P+Q*Q

        * blsk    R10, putspace
        * mov     @vI, R1
        * li      R0, hibyte('I')
        * blsk    R10, print
        * mov     @vA, R1
        * li      R0, hibyte('A')
        * blsk    R10, print
        * mov     @vB, R1
        * li      R0, hibyte('B')
        * blsk    R10, print
        * mov     @vP, R1
        * li      R0, hibyte('P')
        * blsk    R10, print
        * mov     @vQ, R1
        * li      R0, hibyte('Q')
        * blsk    R10, print
        * mov     @vT, R1
        * li      R0, hibyte('T')
        * blsk    R10, print
        * blsk    R10, newline

        mov     @vT, R0
        ci      R0, 4
        jgt     print_i         if 4<P*P+Q*Q
        inc     @vI             I+=1
        mov     @vI, R0
        ci      R0, 16
        jhe     print_space
        b       @loop_i         if I<16
print_space:
        li      R0, >2000
        jmp     print_char
print_table:
        data    '01'
        data    '23'
        data    '45'
        data    '67'
        data    '89'
        data    'AB'
        data    'CD'
        data    'EF'
print_i:
        mov     @vI, R1
        movb    @print_table(R1), R0
print_char:
        blsk    R10, putchar

        * mov     R0, R2
        * li      R0, hibyte('@')
        * blsk    R10, putchar
        * li      R0, hibyte('=')
        * blsk    R10, putchar
        * mov     R2, R0
        * blsk    R10, putchar
        * blsk    R10, newline

        blsk    R10, getchar
        jnc     next_x
        soc     R0, R0
        jne     next_x
        xop     R15,15          halt to system
next_x:
        inc     @vX             X+=1
        mov     @vX, R0
        ci      R0, 30
        jeq     next_y
        b       @loop_x         if X<30
next_y:
        blsk    R10, newline
        inc     @vY             Y+=1
        mov     @vY, R0
        ci      R0, 13
        jeq     mandelbrot_end
        b       @loop_y         if Y<13
mandelbrot_end:
        bind    *R10+
