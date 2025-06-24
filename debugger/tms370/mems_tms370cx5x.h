#ifndef __MEMS_TMS370Cx5x_H__
#define __MEMS_TMS370Cx5x_H__

#include "mems_tms370.h"

namespace debugger {
namespace tms370cx5x {

struct PinsTms370Cx5x;

struct MemsTms370Cx5x final : tms370::MemsTms370 {
    MemsTms370Cx5x(tms370::RegsTms370 *regs, PinsTms370Cx5x *pins, Devs *devs);
    bool internal(uint32_t addr) const override;
};

}  // namespace tms370cx5x
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
