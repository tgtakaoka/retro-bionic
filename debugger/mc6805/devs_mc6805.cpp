#include "devs_mc6805.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"

namespace debugger {
namespace mc6805 {

DevsMc6805::DevsMc6805(uint16_t acia_base)
    : _acia(new Mc6850()) {
    _acia->setBaseAddr(acia_base);
}

DevsMc6805::~DevsMc6805() {
    delete _acia;
}

void DevsMc6805::reset() {
    _acia->reset();
}

void DevsMc6805::begin() {
    enableDevice(_acia);
}

void DevsMc6805::loop() {
    _acia->loop();
}

bool DevsMc6805::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsMc6805::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsMc6805::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsMc6805::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    return Devs::nullDevice();
}

void DevsMc6805::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
}

void DevsMc6805::printDevices() const {
    printDevice(_acia);
}

}  // namespace mc6805
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
