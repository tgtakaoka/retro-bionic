#ifndef __REGS_TMS9980_H__
#define __REGS_TMS9980_H__

#include "regs_tms9900.h"

namespace debugger {
namespace tms9980 {

using tms9900::PinsTms9900Base;

struct RegsTms9980 : tms9900::RegsTms9900 {
    RegsTms9980(tms9900::PinsTms9900Base *pins, Mems *mems);

    const char *cpu() const override;
    void reset() override;

    uint16_t read_reg(uint8_t i) const override;
    void write_reg(uint8_t i, uint16_t data) const override;
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
