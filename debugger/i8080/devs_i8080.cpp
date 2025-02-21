#include "devs_i8080.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"

namespace debugger {
namespace i8080 {

DevsI8080::DevsI8080() : _usart(new I8251()) {}

DevsI8080::~DevsI8080() {
    delete _usart;
}

void DevsI8080::reset() {
    _usart->reset();
    _usart->setBaseAddr(USART_BASE);
}

void DevsI8080::begin() {
    enableDevice(_usart);
}

void DevsI8080::loop() {
    _usart->loop();
}

bool DevsI8080::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsI8080::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsI8080::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

uint16_t DevsI8080::vector() const {
    return _usart->vector();
}

Device *DevsI8080::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    return Devs::nullDevice();
}

void DevsI8080::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
}

void DevsI8080::printDevices() const {
    printDevice(_usart);
}

}  // namespace i8080
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
