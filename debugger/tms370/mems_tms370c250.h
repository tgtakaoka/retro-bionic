#ifndef __MEMS_TMS370C250_H__
#define __MEMS_TMS370C250_H__

#include "mems_tms370.h"

namespace debugger {
namespace tms370c250 {

struct MemsTms370C250 final : tms370::MemsTms370 {
    MemsTms370C250(tms370::RegsTms370 *regs, Devs *devs)
        : MemsTms370(regs, devs) {}
    bool internal(uint32_t addr) const override;
};

}  // namespace tms370c250
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
