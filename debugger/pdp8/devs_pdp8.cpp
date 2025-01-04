#include "devs_pdp8.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"
#include "pins_pdp8.h"

namespace debugger {
namespace pdp8 {

DevsPdp8::DevsPdp8() : _acia(new Mc6850()) {}

DevsPdp8::~DevsPdp8() {
    delete _acia;
}

void DevsPdp8::reset() {
    _acia->reset();
#if defined(ACIA_IO)
    _acia->setBaseAddr(ACIA_IO);
#endif
#if defined(ACIA_ADDR)
    _acia->setBaseAddr(ACIA_ADDR);
#endif
}

void DevsPdp8::begin() {
    enableDevice(_acia);
}

void DevsPdp8::loop() {
    _acia->loop();
}

bool DevsPdp8::isSelected(uint32_t addr) const {
#if defined(ACIA_IO)
    return _acia->isSelected(addr & 0773);
#endif
#if defined(ACIA_ADDR)
    return _acia->isSelected(addr);
#endif
}

uint8_t DevsPdp8::control(uint16_t addr) const {
#if defined(ACIA_IO)
    const auto write = (addr & 4) != 0;
    return IOC_SKIP | 0 | (write ? IOC_C1 : 0) | IOC_C2;
#else
    return IOC_SKIP | IOC_C0 | IOC_C1 | IOC_C2;
#endif
}

uint16_t DevsPdp8::read(uint32_t addr) const {
    if (addr & 4)  // control is for writing
        return 0;
#if defined(ACIA_IO)
    return _acia->read(addr & 0773);
#endif
#if defined(ACIA_ADDR)
    return _acia->read(addr);
#endif
}

void DevsPdp8::write(uint32_t addr, uint16_t data) const {
    if ((addr & 4) == 0)  // control is for reading
        return;
#if defined(ACIA_IO)
    _acia->write(addr & 0773, data);
#endif
#if defined(ACIA_ADDR)
    _acia->write(addr, data);
#endif
}

Device *DevsPdp8::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    return Devs::nullDevice();
}

void DevsPdp8::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
}

void DevsPdp8::printDevices() const {
    printDevice(_acia);
}

}  // namespace pdp8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
