#include "devs_tlcs90.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"
#include "tlcs90_uart_handler.h"

namespace debugger {
namespace tlcs90 {

Tlcs90UartHandler UartH;

DevsTlcs90 Devs;

DevsTlcs90::DevsTlcs90()
    : _usart(new I8251())
#if defined(ENABLE_SERIAL_HANDLER)
      ,
      _uart(new Tlcs90UartHandler())
#endif
{
}

DevsTlcs90::~DevsTlcs90() {
    delete _usart;
#if defined(ENABLE_SERIAL_HANDLER)
    delete _uart;
#endif
}

void DevsTlcs90::reset() {
#if defined(ENABLE_SERIAL_HANDLER)
    _uart->reset();
#endif
    _usart->reset();
    _usart->setBaseAddr(USART_BASE);
}

void DevsTlcs90::begin() {
    enableDevice(_usart);
}

void DevsTlcs90::loop() {
    _usart->loop();
}

void DevsTlcs90::setIdle(bool idle) {
#if defined(ENABLE_SERIAL_HANDLER)
    _uart->setIdle(idle);
#endif
}

bool DevsTlcs90::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsTlcs90::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsTlcs90::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

Device *DevsTlcs90::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
#if defined(ENABLE_SERIAL_HANDLER)
    if (strcasecmp(name, _uart->name()) == 0)
        return _uart;
#endif
    return Devs::nullDevice();
}

void DevsTlcs90::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
#if defined(ENABLE_SERIAL_HANDLER)
    _uart->enable(dev == _uart);
#endif
}

void DevsTlcs90::printDevices() const {
    printDevice(_usart);
#if defined(ENABLE_SERIAL_HANDLER)
    printDevice(_uart);
#endif
}

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
