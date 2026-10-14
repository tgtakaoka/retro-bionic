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

/// Signed 12-bit multiply; R0 *= multiplier
/// @param R0 multiplicand
/// @param AC multiplier
/// @return R0 product
/// @return AC=0
/// @clobber R1 R2 arith_ptr
mul12,          .-.
        dca     R2L             / low(multiplier)
        dca     R2H             / high(multiplier)
        tad     R2L
        spa cla                 / skip if multiplier>=0, AC=0
        jmp     mul12_negative
        tad     R0L
        dca     R1L
        tad     R0H
        dca     R1H             / R1=multiplicand
        jmp     mul12_multiply
mul12_negative,                 / multiplicand*-multiplier
        tad     R2L
        cia
        dca     R2L             / negate multiplier
        tad     R0L
        cia cll                 / L=1 if low(multiplicand)==0
        dca     R1L
        tad     R0H
        cma
        szl                     / skip if L=0
        iac
        dca     R1H             / R1=-multiplicand
mul12_multiply,
        jms     I I_umul24      / R0 = R1 * R2
        jmp     I mul12         / return

/// Signed 24-bit/Unsigned 12-bit division; quotient = dividend / divisor ... remainder
/// @param R0 dividend
/// @param AC unsigned 12-bit divisor address
/// @return R0 quotient
/// @return R1 remainder
/// @return AC=0
/// @clobber R2 arith_ptr
divu12,         .-.
        dca     R2L             / low(divisor)
        dca     R2H             / high(divisor)
        tad     R0H
        spa cla                 / skip if dividend>=0, AC=0
        jmp     divu12_negative
        tad     R0L
        dca     R1L
        tad     R0H
        dca     R1H             / R1=dividend
        jms     I I_udiv16      / R0 = R1 / R2 ... R1
        jmp     I divu12        / return, AC=0
divu12_negative,
        tad     R0L
        cia cll                 / L=1 if low(dividend)==0
        dca     R1L
        tad     R0H
        cma
        szl                     / skip if L=0
        iac
        dca     R1H             / R1=-dividend
        jms     I I_udiv16      / R0 = R1 / R2 ... R1
        tad     R0L
        cia cll                 / L=1 if low(quotient)==0
        dca     R0L
        tad     R0H
        cma
        szl                     / skip if L=0
        iac
        dca     R0H             / negate quotient, AC=0
        jmp     I divu12        / return, AC=0

/// Mandelbrot draw
mandelbrot_end,
        jmp     I mandelbrot
mandelbrot,     .-.
        cla
        tad     const_m12
        dca     vY              / Y=-12
loop_y,
        cla
        tad     vY
        jms     I I_dep12       / R0=Y
        tad     const_416
        jms     I I_mul12       / R0=Y*416
        tad     const_100
        jms     I I_divu12      / R0=Y*416/100
        tad     R0
        dca     vD              / D=Y*416/100, the same for the whole row
        tad     vD
        spa
        cia
        dca     absD            / |D|
        tad     const_C0
        dca     absC
        tad     const_C0r
        dca     absC_rem        / |C| for X=-49
        tad     const_m49
        dca     vX              / X=-49
        jmp     I I_loop_x
I_loop_x,       loop_x
        decimal
const_C0,       112             / 49*229=112*100+21
const_C0r,      21
        octal

        page
/// Mandelbrot inner loop
/// Each round starts with |A|, |B|, S=|B| mod F and aQ=|A|*Q, where
/// Q=|B|/F, left by the previous round. Products and quotients are
/// formed from magnitudes and take their signs afterwards, which
/// truncates toward zero as the signed divide does.
loop_i,
        tad     absA
        dca     md_x            / x=|A|
        tad     vB
        and     bit_sign
        tad     vA              / AC<0 if A*B<0
        spa cla                 / skip if A*B>=0
        jmp     loop_i_bneg
        tad     vS
        jms     I I_muldivF     / AC=|A*S/F|
        tad     aQ              / AC=|A*Q+A*S/F|
        jmp     loop_i_b
loop_i_bneg,
        tad     vS
        jms     I I_muldivF     / AC=|A*S/F|
        tad     aQ
        cia                     / AC=A*Q+A*S/F
loop_i_b,
        cll ral                 / AC=2*(A*Q+A*S/F)
        tad     vD
        dca     vB              / B=2*(A*Q+A*S/F)+D
        tad     absA
        tad     absB
        dca     md_x            / x=|A|+|B|
        tad     absB
        cia
        tad     absA            / AC=|A|-|B|
        spa
        jmp     loop_i_aneg
        jms     I I_muldivF     / AC=(|A|+|B|)*(|A|-|B|)/F=(A*A-B*B)/F
        jmp     loop_i_a
loop_i_aneg,
        cia
        jms     I I_muldivF     / AC=(|A|+|B|)*(|B|-|A|)/F
        cia                     / AC=(A*A-B*B)/F
loop_i_a,
        tad     vC
        dca     vA              / A=(A*A-B*B)/F+C

//        jms     I I_putspace
//        tad     const_m16
//        cia
//        tad     vI              / I=vI+16
//        jms     I I_debug
//        111                     / char 'I'
//        tad     vA
//        jms     I I_debug
//        101                     / char 'A'
//        tad     vB
//        jms     I I_debug
//        102                     / char 'B'
//        jms     I I_newline

/// T=P*P+Q*Q>4, with P=|A|/F and Q=|B|/F, exactly when P+Q>2
        tad     vB
        spa
        cia
        dca     absB            / |B|
        tad     vA
        spa
        cia
        dca     absA            / |A|
        jms     divB            / S=|B| mod F, aQ=|A|*Q
        tad     vS
        cia
        tad     absB            / AC=Q*F
        tad     absA
        tad     const_m3F       / AC=|A|+Q*F-3*F
        sma cla                 / skip if P+Q<3
        jmp     I I_print_i     / T>4
        isz     vI              / I+=1, skip if I=16
        jmp     loop_i
        jmp     I I_print_space / I=16
I_print_i,      print_i
I_print_space,  print_space
bit_sign,       4000

/// Divide |B| by F
/// @param AC=0
/// @return vS remainder |B| mod F
/// @return aQ |A|*(|B|/F)
divB,           .-.
        dca     aQ
        tad     absB
divB_loop,
        tad     const_mF
        spa                     / skip if no borrow
        jmp     divB_end
        dca     vS
        tad     absA
        tad     aQ
        dca     aQ              / aQ+=|A| for each F taken out
        tad     vS
        jmp     divB_loop
divB_end,
        tad     const_F
        dca     vS              / S=|B| mod F
        jmp     I divB

        page
/// Multiply and divide by F; quotient = multiplicand * multiplier / F
/// @param md_x multiplicand, below 1024
/// @param AC multiplier
/// @return AC quotient, which must fit in 12 bits
/// @clobber MQ L
/// The multiply shifts the product right into MQ as the multiplier
/// leaves it, one octal digit of the multiplier per pass. The high
/// word, held in AC, stays below 2*multiplicand; taking out 2*F while
/// it can leaves a remainder below F to start the divide, and puts
/// those quotient bits at the bottom of MQ. The divide then shifts the
/// product's low bits back out of MQ while quotient bits enter at the
/// bottom, in as many passes. Its remainder may go negative instead of
/// being restored.
md_mul_count,   0               / multiply passes
md_div_count,   0               / divide passes
muldivF,        .-.
        sna                     / skip if multiplier != 0
        jmp     I muldivF       / AC=0
        mql                     / MQ=multiplier
        mqa                     / AC=multiplier
        and     bits_7770
        sna cla                 / skip if multiplier >= 8
        jmp     muldivF_1digit
        mqa
        and     bits_7700
        sna cla                 / skip if multiplier >= 64
        jmp     muldivF_2digits
        mqa
        and     bits_7000
        sna cla                 / skip if multiplier >= 512
        jmp     muldivF_3digits
        tad     const_m1
muldivF_3digits,
        tad     const_m1
muldivF_2digits,
        tad     const_m1
muldivF_1digit,
        tad     const_m1        / AC=-digits
        dca     md_mul_count
        tad     md_mul_count
        dca     md_div_count    / AC=0, the high word
muldivF_mul,
        cll rar                 / high >>= 1, L=its LSB
        swp                     / AC=low, MQ=high
        rar                     / low = L:low>>1, L=multiplier bit
        swp                     / AC=high, MQ=low
        szl                     / skip if multiplier bit=0
        tad     md_x            / high += multiplicand
        cll rar
        swp
        rar
        swp
        szl
        tad     md_x
        cll rar
        swp
        rar
        swp
        szl
        tad     md_x
        isz     md_mul_count
        jmp     muldivF_mul
muldivF_high,                   / AC=high*2 before the last shift
        tad     const_m2F
        spa                     / skip if high*2 >= 2*F
        jmp     muldivF_div
        swp
        iac                     / quotient high bits += 1
        swp
        jmp     muldivF_high
muldivF_div,
        tad     const_2F
        cll rar                 / AC=remainder<F, L=next product bit
muldivF_loop,
        ral                     / remainder = remainder*2+L, L=sign
        szl                     / skip if remainder was not negative
        tad     const_2F
        tad     const_mF        / remainder -= F, L=quotient bit
        swp                     / AC=low, MQ=remainder
        ral                     / quotient bit in, next product bit out
        swp                     / AC=remainder, MQ=low
        ral
        szl
        tad     const_2F
        tad     const_mF
        swp
        ral
        swp
        ral
        szl
        tad     const_2F
        tad     const_mF
        swp
        ral
        swp
        isz     md_div_count
        jmp     muldivF_loop
        acl                     / AC=quotient
        jmp     I muldivF
bits_7770,      7770
bits_7700,      7700
bits_7000,      7000
const_m1,       -1

        page
/// Mandelbrot row: C steps along with X instead of being divided anew
loop_x,
        cla
        tad     absC
        dca     absA            / |A|=|C|
        tad     vX
        sma cla                 / skip if X<0
        jmp     loop_x_cpos
        tad     absC
        cia
        jmp     loop_x_c
loop_x_cpos,
        tad     absC
loop_x_c,
        dca     vC              / C=X*229/100
        tad     vC
        dca     vA              / A=C
/// Step |C| to the next X; |X|*229 moves by 2*100+29
        tad     vX
        spa cla                 / skip if X>=0
        jmp     loop_x_down
        tad     absC_rem
        tad     const_29
        tad     const_m2F
        spa                     / skip if the remainder reached 100
        jmp     loop_x_up2
        dca     absC_rem
        tad     const_3
        jmp     loop_x_step
loop_x_up2,
        tad     const_2F
        dca     absC_rem
        tad     const_2
        jmp     loop_x_step
loop_x_down,
        tad     absC_rem
        tad     const_m29
        sma                     / skip if the remainder went negative
        jmp     loop_x_down2
        tad     const_2F
        dca     absC_rem
        tad     const_m3
        jmp     loop_x_step
loop_x_down2,
        dca     absC_rem
        tad     const_m2
loop_x_step,
        tad     absC
        dca     absC            / |C| for X+1
        tad     vD
        dca     vB              / B=D
        tad     absD
        dca     absB            / |B|
        jms     I I_divB        / S=|B| mod F, aQ=|A|*Q
        tad     const_m16
        dca     vI              / I=0, counted from -16

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

        jmp     I I_loop_i
        decimal
const_29,       29
const_m29,      -29
const_2,        2
const_m2,       -2
const_3,        3
const_m3,       -3
        octal

print_space,
        jms     I I_putspace    / print ' '
        jmp     check_break
print_i,
        tad     vI
        tad     const_p6        / I-10
        spa                     / if I>=10
        jmp     print_dec       / if I<10
        tad     char_A          / I-10+'A'
        jmp     print_char
print_dec,
        tad     const_p10       / I
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

next_x,
        cla
        tad     vX
        iac
        dca     vX              / X+=1
        tad     vX
        tad     const_m30       / X-30
        spa                     / Skip if X-30>=0
        jmp     loop_x          / if X<30
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
I_loop_y,               loop_y
I_mandelbrot_end,       mandelbrot_end
const_p6,       6
const_p10,      12

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
