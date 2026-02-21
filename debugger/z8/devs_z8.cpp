#include "devs_z8.h"
#include <string.h>
#include "i8251.h"

namespace debugger {
namespace z8 {

DevsZ8::DevsZ8(SerialHandler *serial)
    : _usart(new I8251())
#if defined(ENABLE_SERIAL_HANDLER)
      ,
      _serial(serial)
#endif
{
}

DevsZ8::~DevsZ8() {
    delete _usart;
#if defined(ENABLE_SERIAL_HANDLER)
    delete _serial;
#endif
}

void DevsZ8::reset() {
#if defined(ENABLE_SERIAL_HANDLER)
    _serial->reset();
#endif
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
#if defined(ENABLE_SERIAL_HANDLER)
    _serial->setIdle(idle);
#endif
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
#if defined(ENABLE_SERIAL_HANDLER)
    if (strcasecmp(name, _serial->name()) == 0)
        return _serial;
#endif
    return Devs::nullDevice();
}

void DevsZ8::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
#if defined(ENABLE_SERIAL_HANDLER)
    _serial->enable(dev == _serial);
#endif
}

void DevsZ8::printDevices() const {
    printDevice(_usart);
#if defined(ENABLE_SERIAL_HANDLER)
    printDevice(_serial);
#endif
}

}  // namespace z8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
