#ifndef __MEMS_Z8_H__
#define __MEMS_Z8_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace z8 {

struct MemsZ8 : DmaMemory {
    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

protected:
    MemsZ8(Devs *devs);
    Devs *const _devs;
};

}  // namespace z8
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
