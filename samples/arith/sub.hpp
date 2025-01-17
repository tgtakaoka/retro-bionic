#ifndef __SUB_HPP__
#define __SUB_HPP__

#include "add.hpp"
#include "neg.hpp"

inline flags usub16(uint16_t &minuend, const uint16_t subtrahend) {
    auto lo = lo8(minuend);
    auto losub = lo8(subtrahend);
    auto f = neg8(losub);
    auto hisub = hi8(subtrahend);
    hisub = not8(hisub);
    if (f.carry)
        uadd8(hisub, 1);
    f = uadd8(lo, losub);
    auto hi = hi8(minuend);
    f = uadd8(hi, hisub, f.carry);
    minuend = uint16(hi, lo);
    return f;
}

inline flags sub16(int16_t &minuend, const int16_t subtrahend) {
    auto sub = uint16(minuend);
    const auto f = usub16(sub, uint16(subtrahend));
    minuend = int16(sub);
    return f;
}

#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
