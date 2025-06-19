#include "devs_i8096.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"

namespace debugger {
namespace i8096 {

DevsI8096::DevsI8096() : _usart(new I8251()) {}

DevsI8096::~DevsI8096() {
    delete _usart;
}

void DevsI8096::reset() {
    _usart->reset();
    _usart->setBaseAddr(USART);
}

void DevsI8096::begin() {
    enableDevice(_usart);
}

void DevsI8096::loop() {
    _usart->loop();
}

bool DevsI8096::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsI8096::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsI8096::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

Device *DevsI8096::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    return Devs::nullDevice();
}

void DevsI8096::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
}

void DevsI8096::printDevices() const {
    printDevice(_usart);
}

}  // namespace i8096
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
