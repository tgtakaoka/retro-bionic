#include "devs_tms370.h"
#include <string.h>
#include "mc6850.h"

namespace debugger {
namespace tms370 {

DevsTms370::DevsTms370() : _acia(new Mc6850()) {}

DevsTms370::~DevsTms370() {
    delete _acia;
}

void DevsTms370::reset() {
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsTms370::begin() {
    enableDevice(_acia);
}

void DevsTms370::loop() {
    _acia->loop();
}

bool DevsTms370::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsTms370::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsTms370::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsTms370::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    return Devs::nullDevice();
}

void DevsTms370::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
}

void DevsTms370::printDevices() const {
    printDevice(_acia);
}

}  // namespace tms370
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
