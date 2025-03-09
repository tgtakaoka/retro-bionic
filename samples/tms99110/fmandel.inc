*** -*- mode: asm; mode: flyspell-prog* -*-

*** Print variable: "name=variable "
*** @param R2 float variable address
*** @param R0 variable name
*** @clobber R0 R1
fprint:
        dect    R10
        mov     R0, *R10        Push variable name
        lr      *R2             load float variable
        cri                     R0=integer value
        mov     R0, R1
        mov     *R10+, R0       Pop variable name
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

kCA:    single  0.0458
kCB:    single  0.08333

*** 32-bit float variables
vCA:    bss     4
vCB:    bss     4
vA:     bss     4
vB:     bss     4
*** 16-bit integer variables
vT:     bss     2
vY:     bss     2
vX:     bss     2
vI:     bss     2

fmandel:
        li      R0, -12
        mov     R0, @vY         Y=-12
loop_y:
        li      R0, -49
        mov     R0, @vX         X=-49
loop_x:
        cir     @vX
        mov     @vX, R1
        mr      @kCA            X*0.0458
        str     @vCA            CA=X*0.0457
        str     @vA             A=CA
        cir     @vY
        mr      @kCB            Y*0.08333
        str     @vCB            CB=y*0.08333
        str     @vB             B=CB
        clr     @vI             I=0

        * mov     @vY, R1
        * li      R0, hibyte('Y')
        * blsk    R10, print
        * mov     @vX, R1
        * li      R0, hibyte('X')
        * blsk    R10, print
        * li      R2, vC
        * li      R0, hibyte('C')
        * blsk    R10, fprint
        * li      R2, vD
        * li      R0, hibyte('D')
        * blsk    R10, fprint
        * blsk    R10, newline

loop_i:
        lr      @vB
        mr      @vB             B*B
        str     R2              R2:R3=B*B
        lr      @vA
        mr      @vA             A*A
        sr      R2              A*A-B*B
        ar      @vCA            A*A-B*B+CA
        str     R2              R2:R3=tmp=A*A-B*B+CA
        lr      @vA
        ar      @vA             2*A
        mr      @vB             2*A*B
        ar      @vCB            2*A*B+CB
        str     @vB             B=2*A*B+CB
        lr      R2
        str     @vA             A=tmp
        mr      R2              A*A
        str     R2              R2:R3=A*A
        lr      @vB
        mr      @vB             B*B
        ar      R2              A*A+B*B
        cri                     R0=int(A*A+B*B)
        mov     R0, @vT

        * blsk    R10, putspace
        * mov     @vI, R1
        * li      R0, hibyte('I')
        * blsk    R10, print
        * li      R2, vA
        * li      R0, hibyte('A')
        * blsk    R10, fprint
        * li      R2, vB
        * li      R0, hibyte('B')
        * blsk    R10, fprint
        * li      R2, vP
        * li      R0, hibyte('P')
        * blsk    R10, fprint
        * li      R2, vQ
        * li      R0, hibyte('Q')
        * blsk    R10, fprint
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
