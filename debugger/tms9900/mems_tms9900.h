#ifndef __MEMS_TMS9900_H__
#define __MEMS_TMS9900_H__

#include "devs.h"
#include "mems.h"

namespace debugger {
namespace tms9900 {

struct MemsTms9900 : DmaMemory {
    MemsTms9900(Devs *devs) : MemsTms9900(devs, true) {}

    uint32_t maxAddr() const override { return UINT16_MAX; }

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    uint16_t vec_nmi() const { return maxAddr() - 3; }

protected:
    // TMS9980 shares this memory but is byte accessed.
    MemsTms9900(Devs *devs, bool wordAccess);

    Devs *_devs;
};

}  // namespace tms9900
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
