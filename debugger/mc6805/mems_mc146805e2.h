#ifndef __MEMS_MC146805E2_H__
#define __MEMS_MC146805E2_H__

#include "mems_mc6805.h"

namespace debugger {
namespace mc146805e2 {

using mc6805::MemsMc6805;
using mc6805::RegsMc6805;

struct MemsMc146805E2 final : MemsMc6805 {
    MemsMc146805E2(RegsMc6805 *regs) : MemsMc6805(regs, 13) {}

    uint16_t read(uint32_t addr) const override;
    void write(uint32_t addr, uint16_t data) const override;

    bool is_internal(uint16_t addr) const override { return addr < 0x80; }
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
