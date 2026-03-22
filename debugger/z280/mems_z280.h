#ifndef __MEMS_Z280_H__
#define __MEMS_Z280_H__

#include "mems.h"

namespace debugger {
namespace z280 {

struct MemsZ280 : ExtMemory {
    MemsZ280();

    // A16-A23 give a 24-bit physical address space.
    uint32_t maxAddr() const override { return UINT32_C(0xFFFFFF); }

    // Byte addressed, but the Z-BUS transfers words; instruction
    // fetches are always word transactions.
    bool wordAccess() const override { return true; }

    // Word transfers are always even-aligned; round down regardless.
    uint16_t read(uint32_t addr) const override {
        return read_word((addr & ~UINT32_C(1)) >> 1);
    }
    void write(uint32_t addr, uint16_t data) const override {
        write_word((addr & ~UINT32_C(1)) >> 1, data);
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
