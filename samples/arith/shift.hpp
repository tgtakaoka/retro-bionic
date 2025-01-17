#ifndef __SHIFT_HPP__
#define __SHIFT_HPP__

#include <stdint.h>
#include "flags.hpp"

inline flags left8(uint8_t &value, const bool carry = false) {
    struct flags f;
    const uint8_t result = (value << 1) | (carry ? 1 : 0);
    f.zero = (result == 0);
    f.negative = msb8(result);
    f.carry = msb8(value);
    f.overflow = msb8(result) != msb8(value);
    value = result;
    return f;
}

inline flags right8(uint8_t &value, const bool carry = false) {
    struct flags f;
    const uint8_t result = (value >> 1) | (carry ? 0x80 : 0);
    f.zero = (result == 0);
    f.negative = msb8(result);
    f.carry = lsb8(value) != 0;
    f.overflow = msb8(result) != msb8(value);
    value = result;
    return f;
}

inline flags left16(uint16_t &value) {
    auto hi = hi8(value);
    auto lo = lo8(value);
    auto f = left8(lo);
    f = left8(hi, f.carry);
    value = uint16(hi, lo);
    return f;
}

inline flags right16(uint16_t &value) {
    auto hi = hi8(value);
    auto lo = lo8(value);
    auto f = right8(hi);
    f = right8(lo, f.carry);
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
