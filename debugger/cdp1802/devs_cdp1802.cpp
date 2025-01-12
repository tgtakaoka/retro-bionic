#include "devs_cdp1802.h"
#include <string.h>
#include "cdp1802_sci_handler.h"
#include "debugger.h"
#include "mc6850.h"

namespace debugger {
namespace cdp1802 {

DevsCdp1802::DevsCdp1802()
    : _acia(new Mc6850())
#if defined(ENABLE_SERIAL_HANDLER)
    , _sci(new Cdp1802SciHandler())
#endif
{}

DevsCdp1802::~DevsCdp1802() {
    delete _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    delete _sci;
#endif
}

void DevsCdp1802::reset() {
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->reset();
#endif
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsCdp1802::begin() {
    enableDevice(_acia);
}

void DevsCdp1802::loop() {
    _acia->loop();
}

void DevsCdp1802::setIdle(bool idle) {
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->enable(!idle);
#endif
}

bool DevsCdp1802::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsCdp1802::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsCdp1802::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsCdp1802::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    if (strcasecmp(name, _sci->name()) == 0)
        return _sci;
#endif
    return Devs::nullDevice();
}

void DevsCdp1802::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->enable(dev == _sci);
#endif
}

void DevsCdp1802::printDevices() const {
    printDevice(_acia);
#if defined(ENABLE_SERIAL_HANDLER)
    printDevice(_sci);
#endif
}

}  // namespace cdp1802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
