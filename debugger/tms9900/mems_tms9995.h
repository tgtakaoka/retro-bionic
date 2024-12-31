#ifndef __MEMS_TMS9995_H__
#define __MEMS_TMS9995_H__

#include "mems_tms9900.h"

namespace debugger {
namespace tms9995 {

struct PinsTms9995;

struct MemsTms9995 final : tms9900::MemsTms9900 {
    MemsTms9995(PinsTms9995 *pins, Devs *devs);

    uint16_t get(uint32_t addr, const char *space = nullptr) const override;
    void put(uint32_t addr, uint16_t data,
            const char *space = nullptr) const override;

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
