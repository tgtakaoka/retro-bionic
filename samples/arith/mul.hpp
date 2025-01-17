#ifndef __MUL_HPP__
#define __MUL_HPP__

#include "add.hpp"
#include "neg.hpp"
#include "shift.hpp"

inline void umul16(uint16_t &multiplicand, uint16_t multiplier) {
    uint16_t product = 0;
    while (multiplier) {
        const auto f = right16(multiplier);
        if (f.carry)
            uadd16(product, multiplicand);
        left16(multiplicand);
    }
    multiplicand = product;
}

inline void mul16(int16_t &_multiplicand, const int16_t _multiplier) {
    uint8_t sign = 0;
    auto multiplicand = uint16(_multiplicand);
    if (msb8(hi8(multiplicand))) {
        uadd8(sign, 1);
        neg16(multiplicand);
    }
    auto multiplier = uint16(_multiplier);
    if (msb8(hi8(multiplier))) {
        uadd8(sign, 1);
        neg16(multiplier);
    }
    umul16(multiplicand, multiplier);
    if (lsb8(sign))
        neg16(multiplicand);
    _multiplicand = int16_t(multiplicand);
}

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
