#include "mems_tms9980.h"

namespace debugger {
namespace tms9980 {

MemsTms9980::MemsTms9980(Devs *devs) : MemsTms9900(devs) {}

uint16_t MemsTms9980::read(uint32_t addr) const {
    return _devs->isSelected(addr) ? _devs->read(addr) : read_byte(addr);
}

void MemsTms9980::write(uint32_t addr, uint16_t data) const {
    if (_devs->isSelected(addr)) {
        _devs->write(addr, data);
    } else {
        write_byte(addr, data);
    }
}

uint16_t MemsTms9980::get_inst(uint32_t addr) const {
    return read16(addr);
}

void MemsTms9980::put_inst(uint32_t addr, uint16_t data) const {
    write16(addr, data);
}

}  // namespace tms9980
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
