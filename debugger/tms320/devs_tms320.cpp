#include "devs_tms320.h"
#include <string.h>
#include "mc6850.h"

namespace debugger {
namespace tms320 {

DevsTms320::DevsTms320() : _acia(new Mc6850()) {}

DevsTms320::~DevsTms320() {
    delete _acia;
}

void DevsTms320::reset() {
    _acia->reset();
    _acia->setBaseAddr(ACIA);
}

void DevsTms320::begin() {
    enableDevice(_acia);
}

void DevsTms320::loop() {
    _acia->loop();
}

bool DevsTms320::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsTms320::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsTms320::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsTms320::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    return Devs::nullDevice();
}

void DevsTms320::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
}

void DevsTms320::printDevices() const {
    printDevice(_acia);
}

}  // namespace tms320
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
