#include "devs_tms9900.h"
#include <string.h>
#include "mc6850.h"

namespace debugger {
namespace tms9900 {

DevsTms9900::DevsTms9900() : _acia(new Mc6850(2)) {}

DevsTms9900::~DevsTms9900() {
    delete _acia;
}

void DevsTms9900::reset() {
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsTms9900::begin() {
    enableDevice(_acia);
}

void DevsTms9900::loop() {
    _acia->loop();
}

bool DevsTms9900::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsTms9900::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsTms9900::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsTms9900::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    return Devs::nullDevice();
}

void DevsTms9900::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
}

void DevsTms9900::printDevices() const {
    printDevice(_acia);
}

}  // namespace tms9900
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
