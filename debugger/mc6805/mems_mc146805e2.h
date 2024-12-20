#ifndef __MEMS_MC146805E2_H__
#define __MEMS_MC146805E2_H__

#include "mems_mc6805.h"

namespace debugger {
namespace mc146805e2 {

struct MemsMc146805E2 final : mc6805::MemsMc6805 {
    MemsMc146805E2(mc6805::RegsMc6805 *regs, Devs *devs)
        : MemsMc6805(regs, devs, 13) {}

    bool is_internal(uint16_t addr) const override { return addr < 0x80; }
};

}  // namespace mc146805e2
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
