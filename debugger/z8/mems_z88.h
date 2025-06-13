#ifndef __MEMS_Z88_H__
#define __MEMS_Z88_H__

#include "mems_z8.h"

namespace debugger {
namespace z88 {

struct RegsZ88;

/**
   Register File: 000-0FF (Bank 0)
   Register File: 100-1FF (Bank 1)
   Register File: 200-2FF (Set Two)
   Unified Memory: 0000-FFFF (000-2FF can be read from 10000-102FF)
 */

struct MemsZ88 final : z8::MemsZ8 {
    MemsZ88(RegsZ88 *regs, Devs *devs);
    uint32_t maxData() const override { return maxAddr() + 0x300; }
    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;
private:
    RegsZ88 *const _regs;
};

}  // namespace z88
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
