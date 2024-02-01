#ifndef __REGS_MC146805E2_H__
#define __REGS_MC146805E2_H__

#include "regs_mc6805.h"

namespace debugger {
namespace mc146805e2 {

using mc6805::MemsMc6805;
using mc6805::PinsMc6805;
using mc6805::RegsMc6805;

struct RegsMc146805E2 final : RegsMc6805 {
    RegsMc146805E2(PinsMc6805 *pins, MemsMc6805 *mems)
        : RegsMc6805(pins, mems) {}

    const char *cpu() const override { return "MC146805"; }
    const char *cpuName() const override { return "MC146805E2"; }
};

extern struct RegsMc146805E2 Regs;

}  // namespace mc146805e2
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
