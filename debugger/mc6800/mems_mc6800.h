#ifndef __MEMS_MC6800_H__
#define __MEMS_MC6800_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace mc6800 {

struct RegsMc6800;

struct MemsMc6800 : DmaMemory {
    MemsMc6800(RegsMc6800 *regs, Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

protected:
    RegsMc6800 *const _regs;
    Devs *const _devs;
};

}  // namespace mc6800
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
