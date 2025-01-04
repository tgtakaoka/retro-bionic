/// -*- mode: asm; mode: flyspell-prog; -*-

/// 24-bit addition; summand += addend
/// @param R0 Signed 24-bit summand
/// @param (PC+1) Signed 24-bit addend address
/// @return AC=0
add24,          .-.
        cla cll                 / AC=0 L=0
        tad     I add24         / addend address
        isz     add24           / advance to return address
        dca     arith_ptr       / pointer to low(addend)
        tad     R0L             / AC=low(R0) L=0
        tad     I arith_ptr     / add low(addend)
        dca     R0L             / store low(R0)
        ral                     / AC=carry L=0
        tad     R0H             / AC=high(R0)+carry
        isz     arith_ptr       / advance to high(addend)
        tad     I arith_ptr     / add high(addend)
        dca     R0H             / store high(R0), AC=0
        jmp     I add24         / return

/// 24-bit subtraction; minuend -= subtrahend
/// @param R0 Signed 24-bit minuend
/// @param (PC+1) Signed 24-bit subtrahend address
/// @return AC=0
/// @return L=0 if borrow
sub24,          .-.
        cla
        tad     I sub24         / subtrahend address
        isz     sub24           / advance to return address
        dca     arith_ptr       / pointer to low(subtrahend)
        tad     I arith_ptr     / AC=low(subtrahend)
        stl cia                 / negate to form 13-bit negative
        tad     R0L             / AC=low(R0)-low(subtrahend)
        dca     R0L             / L=0 if borrowed
        ral                     / propagate complemented borrow
        isz     arith_ptr       / advance to high(subtrahend)
        tad     I arith_ptr     / AC=high(subtrahend)
        cia cml                 / form 13-bit negative
        tad     R0H             / add high(R0)
        dca     R0H             / store high(R0), AC=0
        jmp     I sub24         / return

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
        jms     I I_udiv16      / R0=value/10, AC=0
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

        page

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
        cla cll
        tad     R1L
        tad     R0L
        dca     R0L
        ral                     / AC=carry L=0
        tad     R1H
        tad     R0H
        dca     R0H             / product += multiplicand
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

// Unsigned 16-bit division; quotient = dividend / divisor ... remainder
// @param R1 Unsigned 16-bit dividend
// @param R2 Unsigned 16-bit divisor
// @return R0 quotient
// @return R1 remainder
// @return AC=0
// @clobber R2
udiv16_bits,    0
udiv16,         .-.
        cla
        dca     R0L
        dca     R0H             / quitient = 0
        tad     R2L
        sza                     / skip low(divisor) == 0
        jmp     udiv16_setup
        tad     R2H
        sza                     / skip high(divisor) == 0
        jmp     udiv16_setup
        jmp     I udiv16        / return as divide by zero, AC=0
udiv16_setup,
        cla iac                 / AC=1
        dca     udiv16_bits     / bits=1
        jmp     udiv16_prep     / AC=0
udiv16_prep_loop,
        cla cll                 / AC=0 L=0
        tad     R2L
        ral
        dca     R2L
        tad     R2H
        ral
        dca     R2H             / divisor <<= 1, AC=0
        isz     udiv16_bits     / bits++
udiv16_prep,
        tad     R2H
        rtl
        rtl                     / divisor<<=4
        sma cla                 / skip if msb(divisor)=1, AC=0
        jmp     udiv16_prep_loop        / while MSB(divisor)=0
        jmp     udiv16_enter_loop       / AC=0
udiv16_loop,
        dca     udiv16_bits
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
udiv16_enter_loop,
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
        jmp     udiv16_readd    / if dividend < divisor
        isz     R0L             / LSB(quotient)=1
        jmp     udiv16_next
udiv16_readd,
        cla cll                 / AC=0 L=0
        tad     R2L             / low(divisor)
        tad     R1L             / low(dividend)
        dca     R1L             / dividend += divisor
        ral                     / AC=carry L=0
        tad     R2H             / high(divisor)
        tad     R1H             / high(dividend)
        dca     R1H             / dividend += divisor
udiv16_next,
        sta                     / AC=-1
        tad     udiv16_bits     / AC=bits-1
        sza                     / skip if bits == 0
        jmp     udiv16_loop     / while bits != 0
        jmp     I udiv16        / return, AC=0
