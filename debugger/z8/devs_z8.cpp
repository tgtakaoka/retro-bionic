#include "devs_z8.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"

namespace debugger {
namespace z8 {

DevsZ8::DevsZ8(SerialHandler *serial) : _usart(new I8251()), _serial(serial) {}

DevsZ8::~DevsZ8() {
    delete _usart;
    delete _serial;
}

void DevsZ8::reset() {
    _serial->reset();
    _usart->reset();
    _usart->setBaseAddr(USART_BASE);
}

void DevsZ8::begin() {
    enableDevice(_usart);
}

void DevsZ8::loop() {
    _usart->loop();
}

void DevsZ8::setIdle(bool idle) {
    _serial->setIdle(idle);
}

bool DevsZ8::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsZ8::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsZ8::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

Device *DevsZ8::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    if (strcasecmp(name, _serial->name()) == 0)
        return _serial;
    return Devs::nullDevice();
}

void DevsZ8::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
    _serial->enable(dev == _serial);
}

void DevsZ8::printDevices() const {
    printDevice(_usart);
    printDevice(_serial);
}

}  // namespace z8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
