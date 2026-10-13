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
/// @return L=1 if borrow
sub24,          .-.
        cla
        tad     I sub24         / subtrahend address
        isz     sub24           / advance to return address
        dca     arith_ptr       / pointer to low(subtrahend)
        tad     I arith_ptr     / AC=low(subtrahend)
        stl cia                 / negate to form 13-bit negative
        tad     R0L             / AC=low(R0)-low(subtrahend)
        dca     R0L             / L=1 if borrowed
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
/// @clobber R1 R2 arith_ptr
umul24,         .-.
        cla
        dca     R0L
        dca     R0H             / clear product
        tad     R2H
        sna cla                 / Skip if high(multiplier) != 0, AC=0
        jmp     umul24_loop
        tad     R1L
        dca     arith_ptr       / low(multiplicand)
umul24_high,                    / high(multiplier) only adds into high(product)
        tad     R2H
        sna                     / Skip if high(multiplier) != 0
        jmp     umul24_loop     / AC=0
        cll rar
        dca     R2H             / high(multiplier) >>= 1
        snl                     / Skip if LSB=1
        jmp     umul24_high_next
        tad     arith_ptr
        tad     R0H
        dca     R0H             / high(product) += low(multiplicand)
umul24_high_next,
        tad     arith_ptr
        cll ral
        dca     arith_ptr       / low(multiplicand) <<= 1
        jmp     umul24_high
umul24_loop,
        tad     R2L             / low(multiplier)
        sna                     / Skip if low(multiplier) != 0
        jmp     I umul24        / return, AC=0
        cll rar
        dca     R2L             / low(multiplier) >>= 1
        snl                     / Skip if LSB=1
        jmp     umul24_next
        tad     R1L             / L=1
        tad     R0L
        dca     R0L
        cml ral                 / AC=carry L=0
        tad     R1H
        tad     R0H
        dca     R0H             / product += multiplicand
umul24_next,
        tad     R1L
        cll ral
        dca     R1L
        tad     R1H
        ral
        dca     R1H             / multiplicand <<= 1
        jmp     umul24_loop

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
// @clobber R2 arith_ptr
udiv16_bits,    0
udiv16,         .-.
        cla
        dca     R0L
        dca     R0H             / quotient = 0
        tad     R2L
        sna                     / skip if low(divisor) != 0
        tad     R2H
        sna                     / skip if divisor != 0
        jmp     I udiv16        / return as divide by zero, AC=0
        cla iac                 / AC=1
        dca     udiv16_bits     / bits=1, AC=0
udiv16_prep,
        tad     R2L             / low(divisor)
        stl cia                 / negate to form 13-bit negative
        tad     R1L             / low(dividend)
        cla ral                 / propagate complemented borrow
        tad     R2H             / high(divisor)
        cia cml                 / form 13-bit negative
        tad     R1H             / high(dividend)
        szl cla                 / skip if L=0 (no borrow), AC=0
        jmp     udiv16_enter    / until divisor > dividend
        tad     R2H
        spa cla                 / skip if msb(divisor)=0, AC=0
        jmp     udiv16_enter    / until MSB(divisor)=1
        tad     R2L
        cll ral
        dca     R2L
        tad     R2H
        ral
        dca     R2H             / divisor <<= 1, AC=0
        isz     udiv16_bits     / bits++
        jmp     udiv16_prep
udiv16_enter,
        tad     udiv16_bits
        cia
        dca     udiv16_bits     / bits = -bits
        jmp     udiv16_trial
udiv16_loop,
        tad     R2H
        cll rar
        dca     R2H
        tad     R2L
        rar
        dca     R2L             / divisor >>= 1
udiv16_trial,
        tad     R2L             / low(divisor)
        stl cia                 / negate to form 13-bit negative
        tad     R1L             / low(dividend)
        dca     arith_ptr       / low(dividend-divisor)
        ral                     / propagate complemented borrow
        tad     R2H             / high(divisor)
        cia cml                 / form 13-bit negative
        tad     R1H             / AC=high(dividend-divisor)
        szl                     / skip if L=0 (no borrow)
        jmp     udiv16_borrow   / if dividend < divisor
        dca     R1H
        tad     arith_ptr
        dca     R1L             / dividend -= divisor
udiv16_shift,
        tad     R0L
        cml ral                 / shift in complemented borrow
        dca     R0L
        tad     R0H
        ral
        dca     R0H             / quotient <<= 1
        isz     udiv16_bits     / skip if ++bits == 0
        jmp     udiv16_loop     / while bits != 0
        jmp     I udiv16        / return, AC=0
udiv16_borrow,
        cla                     / L=1
        jmp     udiv16_shift
