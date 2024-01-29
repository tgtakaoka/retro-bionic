#include "mems_mc6801.h"
#include "devs_mc6801.h"

#include "debugger.h"

namespace debugger {
namespace mc6801 {

struct MemsMc6801 Memory {
    &Regs
};

uint16_t MemsMc6801::read(uint32_t addr) const {
    return Devices.isSelected(addr) ? Devices.read(addr) : raw_read(addr);
}

void MemsMc6801::write(uint32_t addr, uint16_t data) const {
    if (Devices.isSelected(addr)) {
        Devices.write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t MemsMc6801::get(uint32_t addr, const char *space) const {
    (void)space;
    return addr < 0x100 ? regs().internal_read(addr) : read(addr);
}

void MemsMc6801::put(uint32_t addr, uint16_t data, const char *space) const {
    if (addr < 0x100) {
        regs().internal_write(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace mc6801
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
