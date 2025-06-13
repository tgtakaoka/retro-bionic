#ifndef __MEMS_TMS9995_H__
#define __MEMS_TMS9995_H__

#include "mems_tms9980.h"

namespace debugger {
namespace tms9995 {

struct PinsTms9995;

struct MemsTms9995 final : tms9980::MemsTms9980 {
    MemsTms9995(PinsTms9995 *pins, Devs *devs);

    uint32_t maxAddr() const override { return UINT16_MAX; }

    uint16_t get_data(uint32_t addr) const override;
    void put_data(uint32_t addr, uint16_t data) const override;

private:
    PinsTms9995 *const _pins;
};

}  // namespace tms9995
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
