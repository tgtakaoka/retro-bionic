#include "devs_mc68000.h"
#include <strings.h>
#include "mc6850.h"

namespace debugger {
namespace mc68000 {

DevsMc68000::DevsMc68000(uint32_t addrMask) : _acia(new Mc6850()) {
    _acia->setBaseAddr(ACIA_BASE & addrMask);
}

void DevsMc68000::setAddrMask(uint32_t addrMask) {
    _acia->setBaseAddr(ACIA_BASE & addrMask);
}

DevsMc68000::~DevsMc68000() {
    delete _acia;
}

void DevsMc68000::reset() {
    _acia->reset();
}

void DevsMc68000::begin() {
    enableDevice(_acia);
}

void DevsMc68000::loop() {
    _acia->loop();
}

bool DevsMc68000::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsMc68000::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsMc68000::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

uint16_t DevsMc68000::vector() const {
    return _acia->vector();
}

Device *DevsMc68000::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    return Devs::nullDevice();
}

void DevsMc68000::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
}

void DevsMc68000::printDevices() const {
    printDevice(_acia);
}

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
