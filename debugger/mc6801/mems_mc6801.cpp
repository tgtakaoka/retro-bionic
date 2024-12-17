#include "mems_mc6801.h"
#include "debugger.h"
#include "regs_mc6801.h"

namespace debugger {
namespace mc6801 {

uint16_t MemsMc6801::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : raw_read(addr);
}

void MemsMc6801::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        raw_write(addr, data);
    }
}

uint16_t MemsMc6801::get(uint32_t addr, const char *) const {
    return addr < 0x100 ? _regs->internal_read(addr) : read(addr);
}

void MemsMc6801::put(uint32_t addr, uint16_t data, const char *) const {
    if (addr < 0x100) {
        _regs->internal_write(addr, data);
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
