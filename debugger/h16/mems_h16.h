#ifndef __MEMS_H16_H__
#define __MEMS_H16_H__

#include "mems.h"

namespace debugger {
namespace h16 {

// A byte memory, big-endian: a word's even byte is its high one, on
// D8-D15. Instructions are bytes at any address.
struct MemsH16 final : ExtMemory {
    MemsH16();

    static constexpr uint32_t ADDR_MASK = 0xFFFFFF;

    uint32_t maxAddr() const override { return ADDR_MASK; }

    // A word transfer moves the aligned pair: one 16-bit access of the
    // little-endian array, swapped.
    uint16_t read_bus(uint32_t addr) const {
        return __builtin_bswap16(read_word(addr >> 1));
    }
    void write_bus(uint32_t addr, uint16_t data) const {
        write_word(addr >> 1, __builtin_bswap16(data));
    }
};

}  // namespace h16
}  // namespace debugger
#endif /* __MEMS_H16_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
