#ifndef __MEMS_MC146805E2_H__
#define __MEMS_MC146805E2_H__

#include "mems_mc6805.h"

namespace debugger {
namespace mc146805e2 {

using mc6805::MemsMc6805;
using mc6805::RegsMc6805;

struct MemsMc146805E2 final : MemsMc6805 {
    MemsMc146805E2(RegsMc6805 &regs) : MemsMc6805(regs, 13, 0x80) {}
};

extern struct MemsMc146805E2 Memory;

}  // namespace mc146805e2
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
