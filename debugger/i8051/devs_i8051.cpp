#include "devs_i8051.h"
#include <string.h>
#include "debugger.h"
#include "i8051_uart_handler.h"
#include "i8251.h"

namespace debugger {
namespace i8051 {

I8051UartHandler UartH;

DevsI8051 Devs;

void DevsI8051::reset() {
    UartH.reset();
    USART.reset();
    USART.setBaseAddr(USART_BASE);
}

void DevsI8051::begin() {
    enableDevice(USART);
}

void DevsI8051::loop() {
    USART.loop();
}

void DevsI8051::setIdle(bool idle) {
    UartH.setIdle(idle);
}

bool DevsI8051::isSelected(uint32_t addr) const {
    return USART.isSelected(addr);
}

uint16_t DevsI8051::read(uint32_t addr) const {
    return USART.read(addr);
}

void DevsI8051::write(uint32_t addr, uint16_t data) const {
    USART.write(addr, data);
}

Device &DevsI8051::parseDevice(const char *name) const {
    if (strcasecmp(name, USART.name()) == 0)
        return USART;
    if (strcasecmp(name, UartH.name()) == 0)
        return UartH;
    return Devs::nullDevice();
}

void DevsI8051::enableDevice(Device &dev) {
    USART.enable(&dev == &USART);
    UartH.enable(&dev == &UartH);
}

void DevsI8051::printDevices() const {
    printDevice(USART);
    printDevice(UartH);
}

}  // namespace i8051
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
