#include "devs_mc6801.h"
#include <string.h>
#include "debugger.h"
#include "mc6801_sci_handler.h"
#include "mc6850.h"

namespace debugger {
namespace mc6801 {

DevsMc6801::DevsMc6801()
    : _acia(new Mc6850())
#if defined(ENABLE_SERIAL_HANDLER)
      ,
      _sci(new Mc6801SciHandler())
#endif
{
}

DevsMc6801::~DevsMc6801() {
    delete _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    delete _sci;
#endif
}

void DevsMc6801::reset() {
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->reset();
#endif
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsMc6801::begin() {
    enableDevice(_acia);
}

void DevsMc6801::loop() {
    _acia->loop();
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->loop();
#endif
}

void DevsMc6801::setIdle(bool idle) {
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->setIdle(idle);
#endif
}

bool DevsMc6801::isSelected(uint32_t addr) const {
#if defined(ENABLE_SERIAL_HANDLER)
    return _acia->isSelected(addr) || _sci->isSelected(addr);
#else
    return _acia->isSelected(addr);
#endif
}

uint16_t DevsMc6801::read(uint32_t addr) const {
#if defined(ENABLE_SERIAL_HANDLER)
    return _acia->isSelected(addr) ? _acia->read(addr) : _sci->read(addr);
#else
    return _acia->read(addr);
#endif
}

void DevsMc6801::write(uint32_t addr, uint16_t data) const {
#if defined(ENABLE_SERIAL_HANDLER)
    if (_acia->isSelected(addr)) {
        _acia->write(addr, data);
    } else {
        _sci->write(addr, data);
    }
#else
    _acia->write(addr, data);
#endif
}

Device *DevsMc6801::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    if (strcasecmp(name, _sci->name()) == 0)
        return _sci;
#endif
    return Devs::nullDevice();
}

void DevsMc6801::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->enable(dev == _sci);
#endif
}

void DevsMc6801::printDevices() const {
    printDevice(_acia);
#if defined(ENABLE_SERIAL_HANDLER)
    printDevice(_sci);
#endif
}

}  // namespace mc6801
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
