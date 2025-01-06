#ifndef __MEMS_INS8060_H__
#define __MEMS_INS8060_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace ins8060 {

struct MemsIns8060 final : DmaMemory {
    MemsIns8060(Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }
    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

private:
    Devs *const _devs;
};

}  // namespace ins8060
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
