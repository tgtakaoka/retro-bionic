#ifndef __MEMS_MC68000_H__
#define __MEMS_MC68000_H__

#include "mems.h"

namespace debugger {
namespace mc68000 {

// All 16 MB in EXTMEM; a board with fewer address lines sees it masked to
// them, and what lies above its mask goes to the disk cache. A board with
// all 24 lines lends 800000-FF7FFF instead, keeping the I/O area above:
// that range reads 0 and drops writes, and a bus cycle there is a bus
// error.
struct MemsMc68000 final : ExtMemory {
    static constexpr uint32_t HOLE_BASE = 0x800000;
    static constexpr uint32_t HOLE_END = 0xFF8000;

    MemsMc68000(uint32_t addrMask);

    uint32_t maxAddr() const override { return _addrMask; }
    uint32_t wrap(uint32_t addr) const { return addr & _addrMask; }
    // One compare on the bus loop's path: no hole is a size of 0.
    bool inHole(uint32_t addr) const { return addr - HOLE_BASE < _holeSize; }
    uint16_t read_byte(uint32_t addr) const override {
        return inHole(addr) ? 0 : ExtMemory::read_byte(addr);
    }
    void write_byte(uint32_t addr, uint16_t data) const override {
        if (!inHole(addr))
            ExtMemory::write_byte(addr, data);
    }
    uint16_t read_word(uint32_t addr) const override {
        return inHole(addr << 1) ? 0 : ExtMemory::read_word(addr);
    }
    void write_word(uint32_t addr, uint16_t data) const override {
        if (!inHole(addr << 1))
            ExtMemory::write_word(addr, data);
    }
    Area cacheArea() const override {
        return _holeSize ? extArea(HOLE_BASE, HOLE_END) : ExtMemory::cacheArea();
    }
    uint16_t read(uint32_t addr) const override { return read16(addr & ~1); }
    void write(uint32_t addr, uint16_t data) const override {
        write16(addr & ~1, data);
    }
    // A word cycle moves the aligned pair: one 16-bit access of the
    // little-endian array, swapped.
    uint16_t read_bus(uint32_t addr) const {
        return __builtin_bswap16(read_word(addr >> 1));
    }
    void write_bus(uint32_t addr, uint16_t data) const {
        write_word(addr >> 1, __builtin_bswap16(data));
    }
    uint32_t read32(uint32_t addr) const {
        return static_cast<uint32_t>(read16(wrap(addr))) << 16 |
               read16(wrap(addr + 2));
    }
#ifdef WITH_DISASSEMBLER
    // The length of the instruction at |addr|, at least 2.
    uint_fast8_t insnLength(uint32_t addr) const;
#endif

private:
    const uint32_t _addrMask;
    const uint32_t _holeSize;
};

}  // namespace mc68000
}  // namespace debugger
#endif /* __MEMS_MC68000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
