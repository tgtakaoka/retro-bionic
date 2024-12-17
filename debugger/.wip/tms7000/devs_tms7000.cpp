#include "devs_tms7000.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"

namespace debugger {
namespace tms7000 {

DevsTms7000 Devs;

void DevsTms7000::reset() {
    ACIA.reset();
    ACIA.setBaseAddr(ACIA_BASE);
    if (_serial)
        _serial->reset();
}

void DevsTms7000::begin() {
    enableDevice(ACIA);
}

void DevsTms7000::loop() {
    ACIA.loop();
}

void DevsTms7000::setIdle(bool idle) {
    if (_serial)
        _serial->setIdle(idle);
}

bool DevsTms7000::isSelected(uint32_t addr) const {
    return ACIA.isSelected(addr);
}

uint16_t DevsTms7000::read(uint32_t addr) const {
    return ACIA.read(addr);
}

void DevsTms7000::write(uint32_t addr, uint16_t data) const {
    ACIA.write(addr, data);
}

Device &DevsTms7000::parseDevice(const char *name) const {
    if (strcasecmp(name, ACIA.name()) == 0)
        return ACIA;
    if (_serial && strcasecmp(name, _serial->name()) == 0)
        return *_serial;
    return Devs::nullDevice();
}

void DevsTms7000::enableDevice(Device &dev) {
    ACIA.enable(&dev == &ACIA);
    if (_serial)
        _serial->enable(&dev == _serial);
}

void DevsTms7000::printDevices() const {
    printDevice(ACIA);
    if (_serial)
        printDevice(*_serial);
}

}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
