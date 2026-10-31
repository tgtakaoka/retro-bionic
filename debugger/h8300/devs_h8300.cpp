#include "devs_h8300.h"
#include <strings.h>
#include "h8300_sci_handler.h"
#include "mc6850.h"

namespace debugger {
namespace h8300 {

DevsH8300::DevsH8300()
    : _acia(new Mc6850())
#if defined(ENABLE_SERIAL_HANDLER)
      ,
      _sci(new H8300SciHandler())
#endif
{
    _acia->setBaseAddr(ACIA_BASE);
}

DevsH8300::~DevsH8300() {
    delete _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    delete _sci;
#endif
}

void DevsH8300::reset() {
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->reset();
#endif
    _acia->reset();
}

void DevsH8300::begin() {
    enableDevice(_acia);
}

void DevsH8300::loop() {
    _acia->loop();
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->pace();
#endif
}

void DevsH8300::setIdle(bool idle) {
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->setIdle(idle);
#endif
}

bool DevsH8300::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsH8300::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsH8300::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsH8300::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    if (strcasecmp(name, _sci->name()) == 0)
        return _sci;
#endif
    return Devs::nullDevice();
}

void DevsH8300::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->enable(dev == _sci);
#endif
}

void DevsH8300::printDevices() const {
    printDevice(_acia);
#if defined(ENABLE_SERIAL_HANDLER)
    printDevice(_sci);
#endif
}

}  // namespace h8300
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
