#ifndef __FLAGS_HPP__
#define __FLAGS_HPP__

#include <stdint.h>

struct flags {
    flags() : zero(false), negative(false), carry(false), overflow(false) {}

    bool zero;
    bool negative;
    bool carry;
    bool overflow;

    const char *str() const {
        static char buf[8] = "[....]";
        buf[1] = zero ? '0' : ' ';
        buf[2] = negative ? '-' : '+';
        buf[3] = carry ? 'C' : ' ';
        buf[4] = overflow ? 'V' : ' ';
        return buf;
    };
};

inline uint8_t msb8(const uint8_t v) {
    return (v & UINT8_C(1 << 7)) ? 1 : 0;
}

inline uint8_t lsb8(const uint8_t v) {
    return (v & UINT8_C(1)) ? 1 : 0;
}

inline uint8_t not8(const uint8_t v) {
    return ~v;
}

inline uint8_t lo8(const uint16_t v) {
    return v & UINT8_MAX;
}

inline uint8_t hi8(const uint16_t v) {
    return (v >> 8) & UINT8_MAX;
}

inline uint16_t uint16(const uint8_t hi, const uint8_t lo) {
    return (static_cast<uint16_t>(hi) << 8) | lo;
}

inline uint16_t uint16(const int16_t v) {
    return static_cast<uint16_t>(v);
}

inline int16_t int16(const uint8_t hi, const uint8_t lo) {
    return static_cast<int16_t>(uint16(hi, lo));
}

inline int16_t int16(const uint16_t v) {
    return static_cast<int16_t>(v);
}

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
