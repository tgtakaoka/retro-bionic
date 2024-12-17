#include "devs_tms7000.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"
#include "pins_tms7000.h"
#include "tms7002_serial_handler.h"

namespace debugger {
namespace tms7000 {

DevsTms7000::DevsTms7000() : _acia(new Mc6850()), _serial(nullptr) {}

DevsTms7000::~DevsTms7000() {
    delete _acia;
    delete _serial;
}

void DevsTms7000::addSerialHandler(HardwareType type) {
    if (_serial == nullptr)
        _serial = new tms7002::Tms7002SerialHandler(type == HW_TMS7001);
}

void DevsTms7000::serialLoop() const {
    if (_serial)
        _serial->loop();
}

void DevsTms7000::reset() {
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
    if (_serial)
        _serial->reset();
}

void DevsTms7000::begin() {
    enableDevice(_acia);
}

void DevsTms7000::loop() {
    _acia->loop();
}

void DevsTms7000::setIdle(bool idle) {
    if (_serial)
        _serial->setIdle(idle);
}

bool DevsTms7000::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsTms7000::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsTms7000::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsTms7000::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    if (_serial && strcasecmp(name, _serial->name()) == 0)
        return _serial;
    return Devs::nullDevice();
}

void DevsTms7000::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
    if (_serial)
        _serial->enable(dev == _serial);
}

void DevsTms7000::printDevices() const {
    printDevice(_acia);
    if (_serial)
        printDevice(_serial);
}

}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
