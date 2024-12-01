#ifndef __MEMS_MC68HC05C0_H__
#define __MEMS_MC68HC05C0_H__

#include "mems_mc6805.h"

namespace debugger {
namespace mc68hc05c0 {

struct MemsMc68HC05C0 final : mc6805::MemsMc6805 {
    MemsMc68HC05C0(mc6805::RegsMc6805 *regs, Devs *devs)
        : MemsMc6805(regs, devs, 16) {}

    bool is_internal(uint16_t addr) const override;
};

}  // namespace mc68hc05c0
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
