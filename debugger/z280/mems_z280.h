#ifndef __MEMS_Z280_H__
#define __MEMS_Z280_H__

#include "mems.h"

namespace debugger {
namespace z280 {

struct MemsZ280 : ExtMemory {
    MemsZ280();

    // A16-A23 give a 24-bit physical address space.
    uint32_t maxAddr() const override { return UINT32_C(0xFFFFFF); }

    // A plain little-endian byte memory, as the CPU sees it. Only the
    // Z-BUS word transfer is odd: the even-address byte rides AD8-15 and
    // the odd-address byte AD0-7. A word transfer is always even-aligned.
    uint16_t read_zbus(uint32_t addr) const {
        return uint16(read_byte(addr), read_byte(addr + 1));
    }
    void write_zbus(uint32_t addr, uint16_t data) const {
        write_byte(addr, hi(data));
        write_byte(addr + 1, lo(data));
    }
};

}  // namespace z280
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
