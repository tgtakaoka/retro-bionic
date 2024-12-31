#include "mems_tms9995.h"
#include "pins_tms9995.h"

namespace debugger {
namespace tms9995 {

MemsTms9995::MemsTms9995(PinsTms9995 *pins, Devs *devs)
    : tms9900::MemsTms9900(16, devs), _pins(pins) {}

uint16_t MemsTms9995::get(uint32_t addr, const char *space) const {
    return _pins->is_internal(addr) ? _pins->internal_read(addr)
                                    : MemsTms9900::get(addr, space);
}

void MemsTms9995::put(uint32_t addr, uint16_t data, const char *space) const {
    if (_pins->is_internal(addr)) {
        _pins->internal_write(addr, data);
    } else {
        MemsTms9900::put(addr, data, space);
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
