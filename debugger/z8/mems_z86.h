#ifndef __MEMS_Z86_H__
#define __MEMS_Z86_H__

#include "mems_z8.h"

namespace debugger {
namespace z86 {

struct RegsZ86;

/**
   Register File:  00-FF
   Unified Memory: 0000-FFFF (00-FF can be read from 10000-100FF)
 */

struct MemsZ86 final : z8::MemsZ8 {
    MemsZ86(RegsZ86 *regs, Devs *dev);
    uint32_t maxData() const override { return maxAddr() + 0x100; }
    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;

    ProtectArea *protectArea() override { return &_rom; }

private:
    RegsZ86 *const _regs;
    ProtectArea _rom;
};

}  // namespace z86
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
