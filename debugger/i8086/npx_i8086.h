#ifndef __NPX_I8086_H__
#define __NPX_I8086_H__

#include <stdint.h>

namespace debugger {
namespace i8086 {

/**
 * The 94 bytes FSAVE stores (8087 manual, Figure 8): the control, status
 * and tag words, the instruction and data pointers, then ST(0) to ST(7)
 * as 10-byte temporary reals, in stack order.
 */
struct NpxImage {
    static constexpr uint8_t SIZE = 94;
    uint8_t bytes[SIZE];

    uint16_t word(uint8_t off) const {
        return bytes[off] | (uint16_t(bytes[off + 1]) << 8);
    }
    uint16_t cw() const { return word(0); }
    uint16_t sw() const { return word(2); }
    uint16_t tw() const { return word(4); }
    uint8_t top() const { return (sw() >> 11) & 7; }
    uint32_t ip() const { return word(6) | (uint32_t(word(8) >> 12) << 16); }
    uint16_t opcode() const { return word(8) & 0x7FF; }
    uint32_t op() const { return word(10) | (uint32_t(word(12) >> 12) << 16); }
    // The tag word goes by physical register; ST(i) is TOP+i.
    uint8_t tag(uint8_t i) const { return (tw() >> (2 * ((top() + i) & 7))) & 3; }
    bool empty(uint8_t i) const { return tag(i) == 3; }
    // ST(i)'s sign and exponent, and its 64-bit significand.
    uint16_t exp(uint8_t i) const { return word(14 + 10 * i + 8); }
    uint64_t sig(uint8_t i) const {
        uint64_t v = 0;
        for (int b = 7; b >= 0; --b)
            v = (v << 8) | bytes[14 + 10 * i + b];
        return v;
    }
};

}  // namespace i8086
}  // namespace debugger
#endif /* __NPX_I8086_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
