#ifndef __MEMS_MC6809_H__
#define __MEMS_MC6809_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace mc6809 {

struct MemsMc6809 final : DmaMemory {
    MemsMc6809(Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

protected:
    Devs *const _devs;
};

}  // namespace mc6809
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
