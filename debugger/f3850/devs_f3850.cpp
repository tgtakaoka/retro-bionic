#include "devs_f3850.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"

namespace debugger {
namespace f3850 {

DevsF3850::DevsF3850() : _usart(new I8251()) {}

DevsF3850::~DevsF3850() {
    delete _usart;
}

void DevsF3850::reset() {
    _usart->reset();
    _usart->setBaseAddr(USART_BASE);
}

void DevsF3850::begin() {
    enableDevice(_usart);
}

void DevsF3850::loop() {
    _usart->loop();
}

bool DevsF3850::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsF3850::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsF3850::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

uint16_t DevsF3850::vector() const {
    return _usart->vector();
}

Device *DevsF3850::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    return Devs::nullDevice();
}

void DevsF3850::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
}

void DevsF3850::printDevices() const {
    printDevice(_usart);
}

}  // namespace f3850
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
