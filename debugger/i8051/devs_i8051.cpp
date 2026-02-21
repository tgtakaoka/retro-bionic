#include "devs_i8051.h"
#include <string.h>
#include "i8251.h"

namespace debugger {
namespace i8051 {

DevsI8051::DevsI8051()
    : _usart(new I8251())
#if defined(ENABLE_SERIAL_HANDLER)
      ,
      _uart(new I8051UartHandler())
#endif
{
}

DevsI8051::~DevsI8051() {
    delete _usart;
#if defined(ENABLE_SERIAL_HANDLER)
    delete _uart;
#endif
}

void DevsI8051::reset() {
#if defined(ENABLE_SERIAL_HANDLER)
    _uart->reset();
#endif
    _usart->reset();
    _usart->setBaseAddr(USART_BASE);
}

void DevsI8051::begin() {
    enableDevice(_usart);
}

void DevsI8051::loop() {
    _usart->loop();
}

void DevsI8051::setIdle(bool idle) {
#if defined(ENABLE_SERIAL_HANDLER)
    _uart->setIdle(idle);
#endif
}

bool DevsI8051::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsI8051::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsI8051::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

Device *DevsI8051::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
#if defined(ENABLE_SERIAL_HANDLER)
    if (strcasecmp(name, _uart->name()) == 0)
        return _uart;
#endif
    return Devs::nullDevice();
}

void DevsI8051::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
#if defined(ENABLE_SERIAL_HANDLER)
    _uart->enable(dev == _uart);
#endif
}

void DevsI8051::printDevices() const {
    printDevice(_usart);
#if defined(ENABLE_SERIAL_HANDLER)
    printDevice(_uart);
#endif
}

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
