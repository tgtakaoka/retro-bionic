#include "mems_mc68hc11.h"
#include "mc68hc11_init.h"
#include "regs_mc68hc11.h"

namespace debugger {
namespace mc68hc11 {

uint16_t MemsMc68hc11::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsMc68hc11::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t MemsMc68hc11::get_data(uint32_t addr) const {
    if (_init.is_internal(addr))
        return regs()->internal_read(addr);
    return read(addr);
}

void MemsMc68hc11::put_data(uint32_t addr, uint16_t data) const {
    if (_init.is_internal(addr)) {
        regs()->internal_write(addr, data);
    } else {
        write(addr, data);
    }
}

}  // namespace mc68hc11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
