/// -*- mode: asm; mode: flyspell-prog* -*-

/// Deposit 12-bit data to R0 with sign extend
/// @param AC 12-bit data
/// @return R0 24-bit sign extended value
/// @return AC=0
dep12,          .-.
        dca     R0L             / store low(R0)
        tad     R0L
        spa cla                 / Skip if R0L >= 0, AC=0
        sta                     / AC=-1
        dca     R0H             / store high(R0)
        jmp     I dep12         / return, AC=0

/// 12-bit/24-bit addition; summand += addend
/// @param R0 Signed 24-bit summand
/// @param AC Signed 12-bit addend
/// @return R0
/// @return AC=0
/// @clobber R1
add12,          .-.
        dca     R1L             / low(addend)
        dca     R1H             / high(addend)
        tad     R1L
        sma cla                 / Skip if addend < 0, AC=0
        jmp     add12_add
        sta                     / AC=-1
        dca     R1H             / sign extended addend
add12_add,
        tad     R1L
        tad     R0L
        dca     R0L
        ral
        tad     R1H
        tad     R0H
        dca     R0H
        jmp     I add12         / return

/// 12-bit/24-bit subtraction; minuend -= subtrahend
/// @param R0 Signed 24-bit minuend
/// @param AC Signed 12-bit minuend
/// @return AC=0
/// @return L=0 if borrow
/// @clobber R1
sub12,          .-.
        dca     R1L             / low(subtrahend)
        dca     R1H             / high(subtrahend)
        tad     R1H
        sma cla                 / Skip if subtrahend < 0, AC=0
        jmp     sub12_sub
        sta
        dca     R1H             / sign extended subtrahend
sub12_sub,
        tad     R1L             / AC=low(subtrahend)
        stl cia                 / negate to form 13-bit negative
        tad     R0L             / AC=low(R0)-low(subtrahend)
        dca     R0L             / L=0 if borrowed
        ral                     / propagate complemented borrow
        tad     R1L             / AC=high(subtrahend)
        cia cml                 / form 13-bit negative
        tad     R0H             / add high(R0)
        dca     R0H             / store high(R0), AC=0
        jmp     I sub12         / return

/// Signed 12-bit multiply; R0 *= multiplier
/// @param R0 multiplicand
/// @param AC multiplier
/// @return R0 product
/// @return AC=0
/// @clobber R1 R2
mul12_sign,     0
mul12,          .-.
        dca     R2L             / low(multiplier)
        dca     R2H             / high(multiplier)
        dca     mul12_sign      / clear sign
        tad     R2L
        sma                     / skip if multiplier<0
        jmp     mul12_multiplicand
        cia                     / negate low(multiplier)
        dca     R2L
        isz     mul12_sign      / sign++
mul12_multiplicand,
        cla
        tad     R0L
        dca     R1L             / low(multiplicand)
        tad     R0H
        dca     R1H             / high(multiplicand)
        tad     R1H
        sma                     / skip if multiplicand<0
        jmp     mul12_multiply
        isz     mul12_sign      / sign++
        jms     I I_neg24       / negate multiplicand
        R1
mul12_multiply,
        jms     I I_umul24      / R0 = R1 * R2
        tad     mul12_sign
        rar                     / L=LSB(sign)
        snl cla                 / Skip if L=1, AC=0
        jmp     mul12_return
        jms     I I_neg24
        R0                      / negate product
mul12_return,
        jmp     I mul12         / return

/// Signed 24-bit/Unsigned 12-bit division; quotient = dividend / divisor ... remainder
/// @param R0 dividend
/// @param AC unsigned 12-bit divisor address
/// @return R0 quotient
/// @return R1 remainder
/// @return AC=0
/// @clobber R2
divu12_sign,    0
divu12,         .-.
        dca     R2L             / low(divisor)
        dca     R2H             / high(divisor)
        dca     divu12_sign     / clear sign
        tad     R0L
        dca     R1L             / R1=dividend
        tad     R0H
        dca     R1H
        tad     R1H
        sma                     / skip if dividend<0
        jmp     divu12_divide
        isz     divu12_sign     / sign++
        jms     I I_neg24       / negate dividend
        R1
divu12_divide,
        jms     I I_udiv16      / R0 = R1 / R2 ... R1
        tad     divu12_sign
        rar                     / L=LSB(sign)
        snl cla                 / Skip if L=1, AC=0
        jmp     divu12_return
        jms     I I_neg24
        R0                      / negate R0, AC=0
divu12_return,
        jmp     I divu12        / return, AC=0

/// Mandelbrot draw variables
mandelbrot_end,
        jmp     I mandelbrot
mandelbrot,     .-.
        cla
        tad     const_m12
        dca     vY              / Y=-12
loop_y,
        cla
        tad     const_m49
        dca     vX              / X=-49
loop_x,
        cla
        tad     vX
        jms     I I_dep12       / R0=X
        tad     const_229
        jms     I I_mul12       / R0=X*229
        tad     const_100
        jms     I I_divu12      / R0=X*229/100
        tad     R0
        dca     vC              / C=X*229/100
        tad     R0
        dca     vA              / A=C
        tad     vY
        jms     I I_dep12       / R0=Y
        tad     const_416
        jms     I I_mul12       / R0=Y*416
        tad     const_100
        jms     I I_divu12      / R0=Y*416/100
        tad     R0
        dca     vD              / D=Y*416/100
        tad     R0
        dca     vB              / B=D
        dca     vI              / I=0

//        tad     vY
//        jms     I I_debug
//        131                     / char 'Y'
//        tad     vX
//        jms     I I_debug
//        130                     / char 'X'
//        tad     vC
//        jms     I I_debug
//        103                     / char 'C'
//        tad     vD
//        jms     I I_debug
//        104                     / char 'D'
//        jms     I I_newline

loop_i,
        cla
        tad     vB
        jms     I I_dep12       / R0=B
        tad     const_F
        jms     I I_divu12      / R0=B/F
        tad     R0
        dca     vQ              / Q=B/F
        tad     vQ
        cia                     / AC=-Q
        jms     I I_dep12       / R0=-Q
        tad     const_F
        jms     I I_mul12       / R0=-Q*F
        tad     vB
        jms     I I_add12       / R0=B-Q*F
        tad     R0
        dca     vS              / S=B-Q*F
        tad     vB
        jms     I I_dep12       / R0=B
        tad     vB
        jms     I I_mul12       / R0=B*B
        tad     R0L
        dca     R3L
        tad     R0H
        dca     R3H             / R3=B*B
        tad     vA
        jms     I I_dep12       / R0=A
        tad     vA
        jms     I I_mul12       / R0=A*A
        jms     I I_sub24       / R0=A*A-B*B, needs 24-bit
        R3
        tad     const_F
        jms     I I_divu12      / R0=(A*A-B*B)/F, 12-bit is enough
        tad     vC
        jms     I I_add12       / R0=(A*A-B*B)/F+C
        tad     R0
        dca     R3L             / TMP=(A*A-B*B)/F+C, 12-bit is enough
        tad     vA
        jms     I I_dep12       / R0=A
        tad     vQ
        jms     I I_mul12       / R0=A*Q
        tad     R0
        dca     vT              / T=A*Q
        tad     vA
        jms     I I_dep12       / R0=A
        tad     vS
        jms     I I_mul12       / R0=A*S
        tad     const_F
        jms     I I_divu12      / R0=A*S/F
        tad     vT
        jms     I I_add12       / R0=A*Q+A*S/F
        tad     R0
        dca     vT              / T=A*Q+A*S/F
        tad     vT
        tad     vT              / AC=2*(A*Q+A*S/F)
        tad     vD              / AC=2*(A*Q+A*S/F)+D
        dca     vB              / B=2*(A*Q+A*S/F)+D
        tad     R3L             / AC=(A*A-B*B)/F+C
        dca     vA              / A=(A*A-B*B)/F+C
        tad     vA
        jms     I I_dep12       / R0=A
        tad     const_F
        jms     I I_divu12      / R0=A/F
        tad     R0
        dca     vP              / P=A/F
        tad     vP
        jms     I I_mul12       / R0=P*P
        tad     R0
        dca     vT              / T=P*P
        tad     vB
        jms     I I_dep12       / R0=B
        tad     const_F
        jms     I I_divu12      / R0=B/F
        tad     R0
        dca     vQ              / Q=B/F
        tad     vQ
        jms     I I_mul12       / R0=Q*Q
        tad     R0
        tad     vT              / R0=P*P+Q*Q
        dca     vT              / T=P*P+Q*Q

//        jms     I I_putspace
//        cla
//        tad     vI
//        jms     I I_debug
//        111                     / char 'I'
//        tad     vA
//        jms     I I_debug
//        101                     / char 'A'
//        tad     vB
//        jms     I I_debug
//        102                     / char 'B'
//        tad     vP
//        jms     I I_debug
//        120                     / char 'P'
//        tad     vQ
//        jms     I I_debug
//        121                     / char 'Q'
//        tad     vT
//        jms     I I_debug
//        124                     / char 'T'
//        jms     I I_newline
//        cla

        jmp     I I_debug_t
I_debug_t,      debug_t
        page
debug_t,
        tad     vT
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

I_loop_i,       loop_i

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

//        dca     R1L             / save character
//        tad     char_atmark
//        jms     I I_putchar     / print '@'
//        cla
//        tad     char_equal
//        jms     I I_putchar     / print '='
//        cla
//        tad     R1L             / restore character
//        jms     I I_putchar     / print character
//        jms     I I_newline
//        jmp     check_break
//char_atmark,    40H             / char '@'
//char_equal,     3DH             / char '='

check_break,
        jms     I I_getchar
        snl                     / Skip if L=1
        jmp     next_x
        sza                     / Skip if AC=0
        jmp     next_x
        hlt                     / halt to system

I_loop_x,               loop_x
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
I_loop_y,               loop_y
        jmp     I I_mandelbrot_end
I_mandelbrot_end,       mandelbrot_end

/// Print variable: "name=R0 "
/// @param AC variable
/// @param (PC+1) variable name
/// @return AC=0
/// @clobber AC R0 R1 R2
debug,          .-.
        jms     I I_dep12       / R0=variable
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
