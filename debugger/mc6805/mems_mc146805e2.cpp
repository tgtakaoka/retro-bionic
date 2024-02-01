#include "mems_mc146805e2.h"
#include "devs_mc146805e2.h"
#include "regs_mc146805e2.h"

namespace debugger {
namespace mc146805e2 {

struct MemsMc146805E2 Memory {
    &Regs
};

uint16_t MemsMc146805E2::read(uint32_t addr) const {
    return Devices.isSelected(addr) ? Devices.read(addr) : raw_read(addr);
}

void MemsMc146805E2::write(uint32_t addr, uint16_t data) const {
    if (Devices.isSelected(addr)) {
        Devices.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

}  // namespace mc146805e2
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
