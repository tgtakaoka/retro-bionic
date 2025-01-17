#ifndef __ADD_HPP__
#define __ADD_HPP__

#include <stdint.h>
#include "flags.hpp"

inline flags uadd8(
        uint8_t &summand, const uint8_t addend, const bool carry = false) {
    const auto addition = uint16(summand) + uint16(addend) + (carry ? 1 : 0);
    summand = addition;
    struct flags f;
    f.zero = (summand == 0);
    f.negative = msb8(summand);
    f.carry = lsb8(hi8(addition));
    f.overflow =
            msb8(summand) == msb8(addend) && msb8(addend) != msb8(addition);
    return f;
}

inline flags uadd16(uint16_t &summand, const uint16_t addend) {
    auto hi = hi8(summand);
    auto lo = lo8(summand);
    auto f = uadd8(lo, lo8(addend));
    f = uadd8(hi, hi8(addend), f.carry);
    summand = uint16(hi, lo);
    return f;
}

inline flags add16(int16_t &summand, const int16_t addend) {
    auto add = uint16(summand);
    const auto f = uadd16(add, uint16(addend));
    summand = int16(add);
    return f;
}

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
