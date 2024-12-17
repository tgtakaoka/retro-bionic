#include "devs_mos6502.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"

namespace debugger {
namespace mos6502 {

DevsMos6502::DevsMos6502() : _acia(new Mc6850()) {}

DevsMos6502::~DevsMos6502() {
    delete _acia;
}

void DevsMos6502::reset() {
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsMos6502::begin() {
    enableDevice(_acia);
}

void DevsMos6502::loop() {
    _acia->loop();
}

bool DevsMos6502::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsMos6502::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsMos6502::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsMos6502::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    return Devs::nullDevice();
}

void DevsMos6502::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
}

void DevsMos6502::printDevices() const {
    printDevice(_acia);
}

}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
