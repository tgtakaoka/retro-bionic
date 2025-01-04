/// -*- mode: asm; mode: flyspell-prog* -*-

/// Mandelbrot draw variables
vC,     0
        0
vD,     0
        0
vA,     0
        0
vB,     0
        0
vS,     0
        0
vP,     0
        0
vQ,     0
        0
vT,     0
        0
vTMP,   0
        0

        decimal
const_m12,      -12     / 12-bit constant -12
const_m49,      -49     / 12-bit constant -49
const_229,              / 24-bit constant 229
        229
        0
const_100,      100     / 12-bit constant 100
const_416,              / 24-bit constant 416
        416
        0
const_F,                / 12-bit/24-bit constant 50
        50
        0
        octal

mandelbrot_end,
        jmp     I mandelbrot
mandelbrot,     .-.
        jms     I I_set12       / Y=-12
        -14                     / const -12
        vY
loop_y,
        jms     I I_set12       / X=-49
        -61                     / const -49
        vX
loop_x,
        jms     I I_load12      / R0=X
        vX
        jms     I I_mul24       / R0=X*229
        const_229
        jms     I I_div12       / R0=X*229/100
        const_100
        jms     I I_store24     / C=X*229/100
        vC
        jms     I I_store24     / A=C
        vA
        jms     I I_load12      / R0=Y
        vY
        jms     I I_mul24
        const_416               / R0=Y*416
        jms     I I_div12 
        const_100               / R0=Y*416/100
        jms     I I_store24     / D=Y*416/100
        vD
        jms     I I_store24     / B=D
        vB
        jms     I I_set12       / I=0
        0
        vI

/        jms     I I_load12      / R0=Y
/        vY
/        jms     I I_debug
/        131                     / char 'Y'
/        jms     I I_load12      / R0=X
/        vX
/        jms     I I_debug
/        130                     / char 'X'
/        jms     I I_load24      / R0=C
/        vC
/        jms     I I_debug
/        103                     / char 'C'
/        jms     I I_load24      / R0=D
/        vD
/        jms     I I_debug
/        104                     / char 'D'
/        jms     I I_newline

loop_i,
        jms     I I_load24
        vB
        jms     I I_div12 
        const_F                 / R0=B/F
        jms     I I_store24     / Q=B/F
        vQ
        jms     I I_neg24       / R0=-Q
        vQ
        jms     I I_mul24       / R0=-Q*F
        const_F
        jms     I I_add24       / R0=B-Q*F
        vB
        jms     I I_store24     / S=B-B*F
        vS
        jms     I I_load24
        vB
        jms     I I_mul24       / R0=B*B
        vB
        jms     I I_store24
        vTMP                    / TMP=B*B
        jms     I I_load24
        vA
        jms     I I_mul24       / R0=A*A
        vA
        jms     I I_sub24       / R0=A*A-B*B
        vTMP
        jms     I I_div12       / R0=(A*A-B*B)/F
        const_F
        jms     I I_add24       / R0=(A*A-B*B)/F+C
        vC
        jms     I I_store24
        vTMP                    / TMP=(A*A-B*B)/F+C
        jms     I I_load24
        vA
        jms     I I_mul24       / R0=A*Q
        vQ
        jms     I I_store24
        vT                      / T=A*Q
        jms     I I_load24
        vA
        jms     I I_mul24       / R0=A*S
        vS
        jms     I I_div12       / R0=A*S/F
        const_F
        jms     I I_add24       / R0=A*Q+A*S/F
        vT
        jms     I I_store24     / T=A*Q+A*S/F
        vT
        jms     I I_add24       / R0=2*(A*Q+A*S/F)
        vT
        jms     I I_add24       / R0=2*(A*Q+A*S/F)+D
        vD
        jms     I I_store24     / B=2*(A*Q+A*S/F)+D
        vB
        jms     I I_load24
        vTMP
        jms     I I_store24     / A=(A*A-B*B)/F+C
        vA
        jms     I I_div12       / R0=A/F
        const_F
        jms     I I_store24     / P=A/F
        vP
        jms     I I_mul24       / R0=P*P
        vP
        jms     I I_store24     / T=P*P
        vT
        jms     I I_load24      / R0=B
        vB
        jms     I I_div12       / R0=B/F
        const_F
        jms     I I_store24     / Q=B/F
        vQ
        jms     I I_mul24       / R0=Q*Q
        vQ
        jms     I I_add24       / R0=P*P+Q*Q
        vT
        jms     I I_store24     / T=P*P+Q*Q
        vT

/        jms     I I_putspace
/        jms     I I_load12      / R0=I
/        vI
/        jms     I I_debug
/        111                     / char 'I'
/        jms     I I_load24      / R0=A
/        vA
/        jms     I I_debug
/        101                     / char 'A'
/        jms     I I_load24      / R0=B
/        vB
/        jms     I I_debug
/        102                     / char 'B'
/        jms     I I_load24      / R0=P
/        vP
/        jms     I I_debug
/        120                     / char 'P'
/        jms     I I_load24      / R0=Q
/        vQ
/        jms     I I_debug
/        121                     / char 'Q'
/        jms     I I_load24      / R0=T
/        vT
/        jms     I I_debug
/        124                     / char 'T'
/        jms     I I_newline

        jms     I I_load24
        vT                      / R0=T
        tad     R0H             / high(T>=0)
        sza                     / skip if high(T) == 0
        jmp     print_i
        tad     R0L             / low(T)
        tad     const_m5        / T-5
        sma cla                 / skip if T-5<0
        jmp     print_i         / if 4<T (T>=5)
        tad     vI
        iac
        dca     vI              / I+=1
        tad     vI
        tad     const_m16       / I-16
        spa cla                 / skip if I-16>=0
        jmp     I I_loop_i      / if I<16
        jms     I I_putspace    / print ' '
        jmp     check_break

vI,             0
        decimal
const_m5,       -5              / const -5
const_m16,      -16             / const -16
const_m10,      -10             / const -10
const_m30,      -30             / const -30
        octal
I_loop_i,       loop_i
char_A,         101             / char 'A'
char_zero,      060             / char '0'

print_i,
        cla
        tad     vI
        tad     const_m10       / I-10
        spa                     / if I>=10
        jmp     print_dec       / if I<10
        tad     char_A          / I-10+'A'
        jmp     print_char
print_dec,
        cla
        tad     vI
        tad     char_zero       / '0'
print_char,
        jms     I I_putchar

/        dca     R1L             / save character
/        jms     I I_set24       / R0='@':'='
/        100                     / char '@'
/        075                     / char '='
/        tad     R0L
/        jms     I I_putchar     / print '@'
/        cla
/        tad     R0H
/        jms     I I_putchar     / print '='
/        cla
/        tad     R1L             / restore character
/        jms     I I_putchar     / print character
/        jms     I I_newline

check_break,
        jms     I I_getchar
        snl                     / Skip if L=1
        jmp     next_x
        sza                     / Skip if AC=0
        jmp     next_x
        hlt                     / halt to system

vX,             0
vY,             0
        decimal
const_m13,      -13             / const -13
        octal

next_x,
        cla
        tad     vX
        iac
        dca     vX              / X+=1
        tad     vX
        tad     const_m30       / X-30
        spa                     / Skip if X-30>=0
        jmp     I I_loop_x      / if X<30
next_y,
        jms     I I_newline
        cla
        tad     vY
        iac
        dca     vY              / Y+=1
        tad     vY
        tad     const_m13       / Y-13
        spa                     / Skip if Y-13>=0
        jmp     I I_loop_y      / if Y<13
        jmp     I I_mandelbrot_end
I_loop_x,               loop_x
I_loop_y,               loop_y
I_mandelbrot_end,       mandelbrot_end

/// Print variable: "name=R0 "
/// @param (PC+1) variable name
/// @return AC=0
/// @clobber AC R0 R1 R2
debug,          .-.
        cla
        tad     I debug         / AC=name
        isz     debug           / advance to return address
        jms     I I_putchar     / print 'name'
        cla
        tad     char_equal      / char '='
        jms     I I_putchar     / print '='
        jms     I I_print_int24 / print variable
        jms     I I_putspace    / print ' '
        cla                     / AC=0
        jmp     I debug         / return
char_equal,     075             / char '='
