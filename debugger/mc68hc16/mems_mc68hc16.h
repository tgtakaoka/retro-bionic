#ifndef __MEMS_MC68HC16_H__
#define __MEMS_MC68HC16_H__

#include "mems.h"

namespace debugger {
namespace mc68hc16 {

// A byte memory, big-endian: a word's even byte is its high one, on
// D8-D15. Instructions are words, read and written whole. The CPU16
// addresses 1MB; A20-A23 follow A19 on the pins, so only A0-A19 decode.
struct MemsMc68hc16 : ExtMemory {
    MemsMc68hc16();

    static constexpr uint32_t ADDR_MASK = 0xFFFFF;

    uint32_t maxAddr() const override { return ADDR_MASK; }

    uint16_t read(uint32_t addr) const override { return read16(addr & ~1); }
    void write(uint32_t addr, uint16_t data) const override {
        write16(addr & ~1, data);
    }

    // A word transfer moves the aligned pair: one 16-bit access of the
    // little-endian array, swapped.
    uint16_t read_bus(uint32_t addr) const {
        return __builtin_bswap16(read_word(addr >> 1));
    }
    void write_bus(uint32_t addr, uint16_t data) const {
        write_word(addr >> 1, __builtin_bswap16(data));
    }
};

}  // namespace mc68hc16
}  // namespace debugger
#endif /* __MEMS_MC68HC16_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
