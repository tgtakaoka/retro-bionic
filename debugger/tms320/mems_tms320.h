#ifndef __MEMS_TMS320_H__
#define __MEMS_TMS320_H__

#include "mems.h"

namespace debugger {
namespace tms320 {

struct MemsTms320 : DmaMemory {
    MemsTms320();

    uint32_t maxAddr() const override { return UINT16_MAX; }
    bool wordAccess() const override { return true; }

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;
};

}  // namespace tms320
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
