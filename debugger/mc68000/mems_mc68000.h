#ifndef __MEMS_MC68000_H__
#define __MEMS_MC68000_H__

#include "mems.h"

namespace debugger {
namespace mc68000 {

// All 16 MB in EXTMEM; a board with fewer address lines sees it masked to
// them.
struct MemsMc68000 final : ExtMemory {
    MemsMc68000(uint32_t addrMask);

    uint32_t maxAddr() const override { return _addrMask; }
    void setAddrMask(uint32_t addrMask) { _addrMask = addrMask; }
    uint32_t wrap(uint32_t addr) const { return addr & _addrMask; }
    uint16_t read(uint32_t addr) const override { return read16(addr & ~1); }
    void write(uint32_t addr, uint16_t data) const override {
        write16(addr & ~1, data);
    }
    uint32_t read32(uint32_t addr) const {
        return static_cast<uint32_t>(read16(wrap(addr))) << 16 |
               read16(wrap(addr + 2));
    }

private:
    uint32_t _addrMask;
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
