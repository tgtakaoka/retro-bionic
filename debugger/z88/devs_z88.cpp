#include "devs_z88.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"
#include "z88_uart_handler.h"

namespace debugger {
namespace z88 {

Z88UartHandler UartH;

DevsZ88 Devs;

void DevsZ88::reset() {
    UartH.reset();
    USART.reset();
    USART.setBaseAddr(USART_BASE);
}

void DevsZ88::begin() {
    enableDevice(USART);
}

void DevsZ88::loop() {
    USART.loop();
}

bool DevsZ88::isSelected(uint32_t addr) const {
    return USART.isSelected(addr);
}

uint16_t DevsZ88::read(uint32_t addr) const {
    return USART.read(addr);
}

void DevsZ88::write(uint32_t addr, uint16_t data) const {
    USART.write(addr, data);
}

Device &DevsZ88::parseDevice(const char *name) const {
    if (strcasecmp(name, USART.name()) == 0)
        return USART;
    if (strcasecmp(name, UartH.name()) == 0)
        return UartH;
    return Devs::nullDevice();
}

void DevsZ88::enableDevice(Device &dev) {
    USART.enable(&dev == &USART);
    UartH.enable(&dev == &UartH);
}

void DevsZ88::printDevices() const {
    printDevice(USART);
    printDevice(UartH);
}

}  // namespace z88
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
