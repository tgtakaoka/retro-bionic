#ifndef __MEMS_TMS9980_H__
#define __MEMS_TMS9980_H__

#include "mems_tms9900.h"

namespace debugger {
namespace tms9980 {

struct MemsTms9980 : tms9900::MemsTms9900 {
    MemsTms9980(Devs *devs);

    uint32_t maxAddr() const override { return UINT16_C(0x3FFF); }
    bool wordAccess() const override { return false; }

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    uint16_t get_inst(uint32_t addr) const override;
    void put_inst(uint32_t addr, uint16_t data) const override;
};

}  // namespace tms9980
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
