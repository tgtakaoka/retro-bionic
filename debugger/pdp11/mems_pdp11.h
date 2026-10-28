#ifndef __MEMS_PDP11_H__
#define __MEMS_PDP11_H__

#include "mems.h"

namespace debugger {
namespace pdp11 {

// Little-endian words; the CPU reads whole words and writes bytes on
// the half R/#WHB or R/#WLB picks.
struct MemsPdp11 : DmaMemory {
    MemsPdp11();

    uint32_t maxAddr() const override { return UINT16_MAX; }

    uint16_t read(uint32_t addr) const override { return read16(addr & ~1); }
    void write(uint32_t addr, uint16_t data) const override {
        write16(addr & ~1, data);
    }
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __MEMS_PDP11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
