#include "devs_pdp8.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"
#include "pins_pdp8.h"

namespace debugger {
namespace pdp8 {

DevsPdp8::DevsPdp8()
    :
#if defined(ACIA_IOT)
      _acia(new Mc6850(4))
#else
      _acia(new Mc6850())
#endif
{
}

DevsPdp8::~DevsPdp8() {
    delete _acia;
}

void DevsPdp8::reset() {
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsPdp8::begin() {
    enableDevice(_acia);
}

void DevsPdp8::loop() {
    _acia->loop();
}

bool DevsPdp8::isSelected(uint32_t addr) const {
#if defined(ACIA_IOT)
    return _acia->isSelected(addr & 07770);
#else
    return _acia->isSelected(addr);
#endif
}

uint8_t DevsPdp8::control(uint16_t addr) const {
#if defined(ACIA_IOT)
    return (addr & 3) | IOC_CKIP;
#else
    return IOC_C0 | IOC_C1 | IOC_SKIP;
#endif
}

uint16_t DevsPdp8::read(uint32_t addr) const {
#if defined(ACIA_IOT)
    if ((addr & 1) == 0)
        return _acia->read(addr & ~3);
    return 0;
#else
    return _acia->read(addr);
#endif
}

void DevsPdp8::write(uint32_t addr, uint16_t data) const {
#if defined(ACIA_IOT)
    if (addr & 1)
        _acia->write(addr & ~3, data);
#else
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
