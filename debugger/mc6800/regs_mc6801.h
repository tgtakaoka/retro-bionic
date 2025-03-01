#ifndef __REGS_MC6801_H__
#define __REGS_MC6801_H__

#include "regs_mc6802.h"

namespace debugger {
namespace mc6801 {

using mc6800::PinsMc6800Base;
using mc6802::RegsMc6802;

struct RegsMc6801 final : RegsMc6802 {
    RegsMc6801(PinsMc6800Base *pins) : RegsMc6802(pins) {}

    void helpRegisters() const override;
    const RegList *listRegisters(uint_fast8_t n) const override;
    bool setRegister(uint_fast8_t reg, uint32_t value) override;

    SoftwareType checkSoftwareType() override;

private:
    void _d(uint16_t d) {
        _b = lo(d);
        _a = hi(d);
    }
};

}  // namespace mc6801
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
