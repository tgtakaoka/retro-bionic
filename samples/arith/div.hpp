#ifndef __DIV_HPP__
#define __DIV_HPP__

#include "neg.hpp"
#include "shift.hpp"
#include "sub.hpp"

#define NON_RESTORING

inline void udiv16(uint16_t &_dividend, uint16_t divisor, uint16_t &remainder) {
    if (divisor == 0) {
        remainder = _dividend;
        return;
    }
    uint8_t bits = 1;
    while (msb8(hi8(divisor)) == 0) {
        left16(divisor);
        bits++;
    }
    uint16_t quotient = 0;
    auto dividend = _dividend;
    auto borrow = false;
    goto entry;
    do {
        right16(divisor);
        left16(quotient);
    entry:
#if defined(NON_RESTORING)
        if (borrow) {
            borrow = !uadd16(dividend, divisor).carry;
        } else {
            borrow = !usub16(dividend, divisor).carry;
        }
#else
        borrow = !usub16(dividend, divisor).carry;
        if (borrow)
            uadd16(dividend, divisor);
#endif
        if (!borrow)
            uadd16(quotient, 1);
    } while (--bits != 0);
#if defined(NON_RESTORING)
    if (borrow)
        uadd16(dividend, divisor);
#endif
    remainder = dividend;
    _dividend = quotient;
}

inline void div16(
        int16_t &_dividend, const int16_t _divisor, uint16_t &remainder) {
    uint8_t sign = 0;
    auto dividend = uint16(_dividend);
    if (msb8(hi8(dividend))) {
        uadd8(sign, 1);
        neg16(dividend);
    }
    auto divisor = uint16(_divisor);
    if (msb8(hi8(divisor))) {
        uadd8(sign, 1);
        neg16(divisor);
    }
    udiv16(dividend, divisor, remainder);
    if (lsb8(sign))
        neg16(dividend);
    _dividend = int16(dividend);
}

inline void div16(int16_t &dividend, const int16_t divisor) {
    uint16_t remainder;
    div16(dividend, divisor, remainder);
}

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
