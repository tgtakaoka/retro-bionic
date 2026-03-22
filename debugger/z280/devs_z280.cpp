#include "devs_z280.h"
#include <string.h>
#include "i8251.h"

namespace debugger {
namespace z280 {

DevsZ280::DevsZ280() : _usart(new I8251()) {}

DevsZ280::~DevsZ280() {
    delete _usart;
}

void DevsZ280::reset() {
    _usart->reset();
    _usart->setBaseAddr(USART);
}

void DevsZ280::begin() {
    enableDevice(_usart);
}

void DevsZ280::loop() {
    _usart->loop();
}

bool DevsZ280::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsZ280::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsZ280::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

Device *DevsZ280::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    return Devs::nullDevice();
}

void DevsZ280::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
}

void DevsZ280::printDevices() const {
    printDevice(_usart);
}

}  // namespace z280
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
