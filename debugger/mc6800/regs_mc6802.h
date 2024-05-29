#ifndef __REGS_MC6802_H__
#define __REGS_MC6802_H__

#include "regs_mc6800.h"

namespace debugger {
namespace mc6802 {

using mc6800::PinsMc6800;
using mc6800::RegsMc6800;

struct RegsMc6802 : RegsMc6800 {
    RegsMc6802(PinsMc6800 &pins) : RegsMc6800(pins) {}

    const char *cpuName() const override;

    uint8_t internal_read(uint16_t addr) const;
    void internal_write(uint16_t addr, uint8_t data) const;
};

extern struct RegsMc6802 Regs;

}  // namespace mc6802
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
