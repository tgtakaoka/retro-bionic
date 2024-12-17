#include "devs_mc146805e2.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"
#include "pins_mc146805e2.h"

namespace debugger {
namespace mc146805e2 {

DevsMc146805E2::DevsMc146805E2() : _acia(new Mc6850()) {}

DevsMc146805E2::~DevsMc146805E2() {
    delete _acia;
}

void DevsMc146805E2::reset() {
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsMc146805E2::begin() {
    enableDevice(_acia);
}

void DevsMc146805E2::loop() {
    _acia->loop();
}

bool DevsMc146805E2::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsMc146805E2::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsMc146805E2::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsMc146805E2::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    return Devs::nullDevice();
}

void DevsMc146805E2::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
}

void DevsMc146805E2::printDevices() const {
    printDevice(_acia);
}

}  // namespace mc146805e2
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
