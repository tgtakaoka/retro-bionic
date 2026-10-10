#include "devs_mc68hc12.h"
#include <strings.h>
#include "mc6850.h"
#include "mc68hc12_init.h"
#include "mc68hc12_sci_handler.h"

namespace debugger {
namespace mc68hc12 {

DevsMc68hc12::DevsMc68hc12(Mc68hc12Init &init)
    : _init(init),
      _acia(new Mc6850())
#if defined(ENABLE_SERIAL_HANDLER)
      ,
      _sci(new Mc68hc12SciHandler())
#endif
{
}

DevsMc68hc12::~DevsMc68hc12() {
    delete _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    delete _sci;
#endif
}

void DevsMc68hc12::reset() {
    _init.reset();
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->reset();
#endif
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsMc68hc12::begin() {
    enableDevice(_acia);
}

void DevsMc68hc12::loop() {
    _acia->loop();
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->loop();
#endif
}

void DevsMc68hc12::setIdle(bool idle) {
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->setIdle(idle);
#endif
}

bool DevsMc68hc12::isSelected(uint32_t addr) const {
#if defined(ENABLE_SERIAL_HANDLER)
    return _acia->isSelected(addr) || _sci->isSelected(addr);
#else
    return _acia->isSelected(addr);
#endif
}

uint16_t DevsMc68hc12::read(uint32_t addr) const {
#if defined(ENABLE_SERIAL_HANDLER)
    return _acia->isSelected(addr) ? _acia->read(addr) : _sci->read(addr);
#else
    return _acia->read(addr);
#endif
}

void DevsMc68hc12::write(uint32_t addr, uint16_t data) const {
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

Device *DevsMc68hc12::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    if (strcasecmp(name, _sci->name()) == 0)
        return _sci;
#endif
    if (strcasecmp(name, _init.name()) == 0)
        return &_init;
    return Devs::nullDevice();
}

void DevsMc68hc12::enableDevice(Device *dev) {
    if (dev == &_init)
        return;
    _acia->enable(dev == _acia);
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->enable(dev == _sci);
#endif
}

void DevsMc68hc12::printDevices() const {
    printDevice(_acia);
#if defined(ENABLE_SERIAL_HANDLER)
    printDevice(_sci);
#endif
    printDevice(&_init);
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
