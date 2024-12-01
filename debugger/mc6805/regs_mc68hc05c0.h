#ifndef __REGS_MC68HC05C0_H__
#define __REGS_MC68HC05C0_H__

#include "regs_mc6805.h"

namespace debugger {
namespace mc68hc05c0 {

using mc6805::MemsMc6805;
using mc6805::PinsMc6805;
using mc6805::RegsMc6805;

struct RegsMc68HC05C0 final : RegsMc6805 {
    RegsMc68HC05C0(PinsMc6805 &pins, MemsMc6805 &mems)
        : RegsMc6805(pins, mems) {}

    const char *cpu() const override { return "MC68HC05"; }
};

extern struct RegsMc68HC05C0 Regs;

}  // namespace mc68hc05c0
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
