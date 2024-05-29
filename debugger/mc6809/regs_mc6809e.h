#ifndef __REGS_MC6809E_H__
#define __REGS_MC6809E_H__

#include "regs_mc6809.h"

namespace debugger {
namespace mc6809e {

using mc6809::PinsMc6809;
using mc6809::RegsMc6809;
using mc6809::SoftwareType;

struct RegsMc6809E final : RegsMc6809 {
    RegsMc6809E(PinsMc6809 &pins) : RegsMc6809(pins) {}

    const char *cpuName() const override {
        return _type == SoftwareType::SW_MC6809 ? "MC6809E" : "HD6309E";
    }
};

extern struct RegsMc6809E Regs;

}  // namespace mc6809e
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
