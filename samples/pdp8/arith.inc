/// -*- mode: asm; mode: flyspell-prog; -*-

/// Set 12-bit immediate value to destination
/// @param (PC+1) 12-bit immediate value
/// @param (PC+2) destination address
/// @return AC=0
set12,          .-.
        cla
        tad     I set12         / immediate value
        isz     set12           / advance to destination address
        dca     R0L             / save value
        tad     I set12         / destination address
        isz     set12           / advance to return address
        dca     arith_ptr       / pointer to destination
        tad     R0L             / restore value
        dca     I arith_ptr     / store value to destination
        jmp     I set12         / return, AC=0

/// Set 24-bit immediate value to R0
/// @param (PC+1) low 12 bit value
/// @param (PC+2) high 12-bit value
/// @return AC=0
set24,          .-.
        cla
        tad     I set24         / AC=low(value)
        dca     R0L             / store low(R0)
        isz     set24           / advance to high(value)
        tad     I set24         / AC=high(value)
        dca     R0H             / store high(R0), AC=0
        isz     set24           / advance to return address
        jmp     I set24         / return, AC=0

/// Load 12-bit data to R0 with sign extend
/// @param (PC+1) source address
/// @return R0 24-bit sign extended value
/// @return AC=0
load12,         .-.
        cla
        tad     I load12        / source address
        isz     load12          / advance to return address
        dca     arith_ptr       / pointer to source
        tad     I arith_ptr     / 12-bit value
        dca     R0L             / store low(R0)
        tad     R0L
        spa cla                 / Skip if R0L >= 0, AC=0
        sta                     / AC=-1
        dca     R0H             / store high(R0)
        jmp     I load12        / return, AC=0

/// Load 24-bit data to R0
/// @param (PC+1) source address
/// @return AC=0
load24,         .-.
        cla
        tad     I load24        / source address
        isz     load24          / advance to return address
        dca     arith_ptr       / pointer to low(source)
        tad     I arith_ptr     / AC=low(source)
        dca     R0L             / store low(R0)
        isz     arith_ptr       / advance to high(source)
        tad     I arith_ptr     / AC=high(source)
        dca     R0H             / store high(R0), AC=0
        jmp     I load24        / return

/// Store 24-bit data from R0
/// @param (PC+1) destination address
/// @return AC=0
store24,         .-.
        cla
        tad     I store24       / destination address
        isz     store24         / advance to return address
        dca     arith_ptr       / pointer to low(destination)
        tad     R0L             / load low(R0)
        dca     I arith_ptr     / store low(destination)
        tad     R0H             / load high(R0)
        isz     arith_ptr       / advance to high(destination)
        dca     I arith_ptr     / store high(destination), AC=0
        jmp     I store24       / return

/// 24-bit addition; R0 += source
/// @param (PC+1) source address
/// @return AC=0
add24,          .-.
        cla cll                 / AC=0 L=0
        tad     I add24        / source address
        isz     add24           / advance to return address
        dca     arith_ptr       / pointer to low(source)
        tad     R0L             / AC=low(R0) L=0
        tad     I arith_ptr     / add low(source)
        dca     R0L             / store low(R0)
        ral                     / AC=carry L=0
        tad     R0H             / AC=high(R0)+carry
        isz     arith_ptr       / advance to high(source)
        tad     I arith_ptr     / add high(source)
        dca     R0H             / store high(R0), AC=0
        jmp     I add24         / return

/// 24-bit subtraction; R0 -= source
/// @param (PC+1) source address
/// @return AC=0
/// @return L=0 if borrow
sub24,          .-.
        cla
        tad     I sub24         / source address
        isz     sub24           / advance to return address
        dca     arith_ptr       / pointer to low(source)
        tad     I arith_ptr     / AC=low(source)
        stl cia                 / negate to form 13-bit negative
        tad     R0L             / AC=low(R0)-low(source)
        dca     R0L             / L=0 if borrowed
        ral                     / propagate complemented borrow
        isz     arith_ptr       / advance to high(source)
        tad     I arith_ptr     / AC=high(source)
        cia cml                 / form 13-bit negative
        tad     R0H             / add high(R0)
        dca     R0H             / store high(R0), AC=0
        jmp     I sub24         / return

        page

/// Print signed 24-bit integer as decimal
/// @param R0 value
/// @return AC=0
/// @clobber R0 R1 R2
char_minus,     055             / '-'
print_int24,    .-.
        cla
        tad     R0H
        sma cla                 / Skip if R0 < 0, AC=0
        jmp     print_uint24
        tad     char_minus
        jms     I I_putchar     / print '-'
        jms     I I_neg24
        R0                      / negate R0
print_uint24,
        dca     I print_uint24_ptr      / *ptr++ = EOS
        isz     print_uint24_ptr
print_uint24_loop,              / AC=0
        tad     R0L
        dca     R1L
        tad     R0H
        dca     R1H             / R1 = R0 (dividend)
        tad     const_10
        dca     R2L
        dca     R2H             / R2 = 10 (divisor)
        jms     I I_udiv24      / R0=value/10, AC=0
        tad     R1L             / remainder
        tad     char_0          / '0'
        dca     I print_uint24_ptr      / store 'digit'
        isz     print_uint24_ptr        / advance ptr
        tad     R0L             / low(quotient)
        sza cla                 / Skip if low(quotient) == 0, AC=0
        jmp     print_uint24_loop       / while quotient != 0
        tad     R0H             / high(quotient)
        sza cla                 / Skip if high(quotient) == 0, AC=0
        jmp     print_uint24_loop       / while quotient != 0
print_uint24_print,
        sta                     / AC=-1
        tad     print_uint24_ptr        / --ptr
        dca     print_uint24_ptr
        tad     I print_uint24_ptr      / AC=*ptr
        sna                     / Skip if not EOS
        jmp     I print_int24   / return, AC=0, ptr=buf
        jms     I I_putchar     / print 'digit'
        jmp     print_uint24_print

char_0,         060             / char '0'
const_10,       012             / const 10
print_uint24_ptr,
        print_uint24_buf        / string pointer
print_uint24_buf,               / buffer, UINT24_MAX=16777215
        *(.+11)                 / 9 words (8 chars + EOS)

/// Unsigned 24-bit multiply; product = multiplicand * multiplier
/// @param R1 multiplicand
/// @param R2 multiplier
/// @return R0 product
/// @return AC=0
/// @clobber R1 R2
umul24,         .-.
        cla
        dca     R0L
        dca     R0H             / clear product
        jmp     umul24_check    / AC=0
umul24_loop,
        cla cll                 / AC=0 L=0
        tad     R2H
        rar
        dca     R2H
        tad     R2L
        rar
        dca     R2L             / multiplier >>= 1
        snl                     / Skip LSB=1
        jmp     umul24_next
        jms     I I_add24
        R1                      / product += multiplicand
umul24_next,
        cla cll                 / AC=0 L=0
        tad     R1L
        ral
        dca     R1L
        tad     R1H
        ral
        dca     R1H             / multiplicand <<= 1
umul24_check,
        tad     R2L             / low(multiplier)
        sza cla                 / Skip if low(multiplier) == 0, AC=0
        jmp     umul24_loop     / while until multiplier != 0
        tad     R2H             / high(multiplier)
        sza cla                 / Skip if high(multiplier) == 0, AC=0
        jmp     umul24_loop     / while until multiplier != 0
        jmp     I umul24        / return, AC=0

/// Signed 24-bit multiply; R0 *= source
/// @param R0 multiplicand
/// @param (PC+1) multiplier source address
/// @return R0 product
/// @return AC=0
/// @clobber R1 R2
mul24_sign,     0
mul24,          .-.
        cla
        dca     mul24_sign      / clear sign
        tad     I mul24         / source address
        isz     mul24           / advance to return address
        dca     arith_ptr       / pointer to low(source)
        tad     I arith_ptr     / low(source)
        dca     R2L             / low(multiplier)
        isz     arith_ptr       / advance to high(source)
        tad     I arith_ptr     / high(source)
        dca     R2H             / high(multiplier)
        tad     R2H
        sma cla                 / skip if multiplier<0, AC=0
        jmp     mul24_multiplicand
        isz     mul24_sign      / sign++
        jms     I I_neg24       / negate multiplier
        R2
mul24_multiplicand,
        tad     R0L
        dca     R1L             / low(multiplicand)
        tad     R0H
        dca     R1H             / high(multiplicand)
        tad     R1H
        sma                     / skip if multiplicand<0
        jmp     mul24_multiply
        isz     mul24_sign      / sign++
        jms     I I_neg24       / negate multiplicand
        R1
mul24_multiply,
        jms     umul24          / R0 = R1 * R2
        tad     mul24_sign
        rar                     / L=LSB(sign)
        snl cla                 / Skip if L=1, AC=0
        jmp     mul24_return
        jms     I I_neg24
        R0                      / negate product
mul24_return,
        jmp     I mul24         / return

        page

/// Negate 24-bit; source = -source
/// @param (PC+1) source address
/// @return AC=0
neg24,          .-.
        cla
        tad     I neg24         / source address
        isz     neg24           / advance to return address
        dca     arith_ptr       / pointer to low(source)
        tad     I arith_ptr     / low(source)
        cia cll                 / negate AC, L=1 if AC=0
        dca     I arith_ptr     / store low(source)
        isz     arith_ptr
        tad     I arith_ptr     / AC=high(source)
        cma                     / ~AC
        szl                     / skip if L=0
        iac                     / AC += L
        dca     I arith_ptr     / store high(source), AC=0
        jmp     I neg24         / return

/// Signed 24bit/12-bit division; quotient = dividend / divisor ... remainder
/// @param R0 dividend
/// @param (PC+1) 12-bit divisor
/// @return R0 quotient
/// @return R1 remainder
/// @return AC=0
/// @clobber R2
div12_sign,     0
div12,          .-.
        cla
        dca     div12_sign      / clear sign
        tad     I div12
        isz     div12
        dca     arith_ptr
        tad     I arith_ptr     / AC=divisor
        sma                     / Skip if divisor < 0
        jmp     div12_divisor_plus
        isz     div12_sign      / sign++
        cia                     / negate divisor
div12_divisor_plus,
        dca     R2L             / R2L=|divisor|
        dca     R2H             / R2H=0
        tad     R0L
        dca     R1L             / R1=dividend
        tad     R0H
        dca     R1H
        tad     R1H
        sma                     / skip if R0H<0
        jmp     div12_divide
        isz     div12_sign      / sign++
        jms     I I_neg24       / negate dividend
        R1
div12_divide,
        jms     udiv24          / R0 = R1 / R2 ... R1
        tad     div12_sign
        rar                     / L=LSB(sign)
        snl cla                 / Skip if L=1, AC=0
        jmp     div12_return
        jms     I I_neg24
        R0                      / negate R0, AC=0
div12_return,
        jmp     I div12         / return, AC=0

// Unsigned 24-bit division; quotient = dividend / divisor ... remainder
// @param R1 dividend
// @param R2 divisor
// @return R0 quotient
// @return R1 remainder
// @return AC=0
// @clobber R2
udiv24_bits,    0
udiv24,         .-.
        cla
        dca     R0L
        dca     R0H             / quitient = 0
        tad     R2L
        sza                     / skip low(divisor) == 0
        jmp     udiv24_setup
        tad     R2H
        sza                     / skip high(divisor) == 0
        jmp     udiv24_setup
        jmp     I udiv24        / return as divide by zero, AC=0
udiv24_setup,
        cla iac                 / AC=1
        dca     udiv24_bits     / bits=1
        jmp     udiv24_prep     / AC=0
udiv24_prep_loop,
        cla cll                 / AC=0 L=0
        tad     R2L
        ral
        dca     R2L
        tad     R2H
        ral
        dca     R2H             / divisor <<= 1, AC=0
        isz     udiv24_bits     / bits++
udiv24_prep,
        tad     R2H
        sma cla                 / skip if MSB(divisor)=1, AC=0
        jmp     udiv24_prep_loop        / while MSB(divisor)=0
        jmp     udiv24_enter_loop       / AC=0
udiv24_loop,
        dca     udiv24_bits
        cla cll                 / AC=0 L=0
        tad     R2H
        rar
        dca     R2H
        tad     R2L
        rar
        dca     R2L             / divisor >= 1
        cla cll                 / AC=0 L=0
        tad     R0L
        ral
        dca     R0L
        tad     R0H
        ral
        dca     R0H             / quotient <<= 1, LSB(quotient)=0
udiv24_enter_loop,
        tad     R2L             / low(divisor)
        stl cia                 / negate to form 13-bit negative
        tad     R1L             / low(dividend)
        dca     R1L             / dividend -= divisor
        ral                     / propagate complemented borrow
        tad     R2H             / high(divisor)
        cia cml                 / form 13-bit negative
        tad     R1H             / high(dividend)
        dca     R1H             / dividend -= divisor
        szl                     / skip if L=0 (no borrow)
        jmp     udiv24_readd    / if dividend < divisor
        isz     R0L             / LSB(quotient)=1
        jmp     udiv24_next
udiv24_readd,
        cla cll                 / AC=0 L=0
        tad     R2L             / low(divisor)
        tad     R1L             / low(dividend)
        dca     R1L             / dividend += divisor
        ral                     / AC=carry L=0
        tad     R2H             / high(divisor)
        tad     R1H             / high(dividend)
        dca     R1H             / dividend += divisor
udiv24_next,
        sta                     / AC=-1
        tad     udiv24_bits     / AC=bits-1
        sza                     / skip if bits == 0
        jmp     udiv24_loop     / while bits != 0
        jmp     I udiv24        / return, AC=0
