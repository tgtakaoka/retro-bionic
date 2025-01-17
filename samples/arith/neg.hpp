#ifndef __NEG_HPP__
#define __NEG_HPP__

#include <stdint.h>
#include "add.hpp"
#include "flags.hpp"

inline flags neg8(uint8_t &value) {
    uint8_t negation = ~value;
    auto f = uadd8(negation, 1);
    f.zero = (negation == 0);
    f.negative = msb8(negation);
    f.overflow = false;
    value = negation;
    return f;
}

inline flags neg16(uint16_t &value) {
    auto lo = lo8(value);
    auto f = neg8(lo);
    auto hi = hi8(value);
    hi = not8(hi);
    if (f.carry)
        uadd8(hi, 1);
    value = uint16(hi, lo);
    return f;
}

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
