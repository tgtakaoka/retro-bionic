#include "devs_mc6800.h"
#include <string.h>
#include "mc6850.h"

namespace debugger {
namespace mc6800 {

DevsMc6800::DevsMc6800() : _acia(new Mc6850()) {}

DevsMc6800::~DevsMc6800() {
    delete _acia;
}

void DevsMc6800::reset() {
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsMc6800::begin() {
    enableDevice(_acia);
}

void DevsMc6800::loop() {
    _acia->loop();
}

bool DevsMc6800::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsMc6800::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsMc6800::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsMc6800::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    return Devs::nullDevice();
}

void DevsMc6800::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
}

void DevsMc6800::printDevices() const {
    printDevice(_acia);
}

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
