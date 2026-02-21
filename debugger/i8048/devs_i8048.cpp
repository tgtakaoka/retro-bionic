#include "devs_i8048.h"
#include <string.h>
#include "i8251.h"

namespace debugger {
namespace i8048 {

DevsI8048::DevsI8048() : _usart(new I8251()) {}

DevsI8048::~DevsI8048() {
    delete _usart;
}

void DevsI8048::reset() {
    _usart->reset();
    _usart->setBaseAddr(USART_BASE);
}

void DevsI8048::begin() {
    enableDevice(_usart);
}

void DevsI8048::loop() {
    _usart->loop();
}

bool DevsI8048::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsI8048::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsI8048::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

Device *DevsI8048::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    return Devs::nullDevice();
}

void DevsI8048::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
}

void DevsI8048::printDevices() const {
    printDevice(_usart);
}

}  // namespace i8048
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
