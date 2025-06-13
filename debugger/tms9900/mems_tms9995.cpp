#include "mems_tms9995.h"
#include "pins_tms9995.h"

namespace debugger {
namespace tms9995 {

MemsTms9995::MemsTms9995(PinsTms9995 *pins, Devs *devs)
    : MemsTms9980(devs), _pins(pins) {}

uint16_t MemsTms9995::get_data(uint32_t addr) const {
    if (_pins->is_internal(addr))
        return _pins->internal_read(addr);
    return MemsTms9900::get_data(addr);
}

void MemsTms9995::put_data(uint32_t addr, uint16_t data) const {
    if (_pins->is_internal(addr)) {
        _pins->internal_write(addr, data);
    } else {
        MemsTms9900::put_data(addr, data);
    }
}

}  // namespace tms9995
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
