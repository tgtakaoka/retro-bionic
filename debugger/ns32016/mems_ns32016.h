#ifndef __MEMS_NS32016_H__
#define __MEMS_NS32016_H__

#include "mems.h"

namespace debugger {
namespace ns32016 {

// Little-endian over 16 MB: a word's even byte is its low one, on AD0-AD7.
struct MemsNs32016 final : ExtMemory {
    MemsNs32016(bool fpu);

    static constexpr uint32_t ADDR_MASK = 0xFFFFFF;
    uint32_t maxAddr() const override { return ADDR_MASK; }

    // The aligned word a bus cycle moves: one 16-bit access of the
    // little-endian array.
    uint16_t read_bus(uint32_t addr) const { return read_word(addr >> 1); }
    void write_bus(uint32_t addr, uint16_t data) const {
        write_word(addr >> 1, data);
    }
    uint32_t read32(uint32_t addr) const {
        return read16(addr) | (uint32_t(read16(addr + 2)) << 16);
    }
};

}  // namespace ns32016
}  // namespace debugger
#endif /* __MEMS_NS32016_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
