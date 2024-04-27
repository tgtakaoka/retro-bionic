#include "devs_tlcs90.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"
#include "tlcs90_uart_handler.h"

namespace debugger {
namespace tlcs90 {

Tlcs90UartHandler UartH;

DevsTlcs90 Devs;

void DevsTlcs90::reset() {
    UartH.reset();
    USART.reset();
    USART.setBaseAddr(USART_BASE);
}

void DevsTlcs90::begin() {
    enableDevice(USART);
}

void DevsTlcs90::loop() {
    USART.loop();
}

void DevsTlcs90::setIdle(bool idle) {
    UartH.setIdle(idle);
}

bool DevsTlcs90::isSelected(uint32_t addr) const {
    return USART.isSelected(addr);
}

uint16_t DevsTlcs90::read(uint32_t addr) const {
    return USART.read(addr);
}

void DevsTlcs90::write(uint32_t addr, uint16_t data) const {
    USART.write(addr, data);
}

Device &DevsTlcs90::parseDevice(const char *name) const {
    if (strcasecmp(name, USART.name()) == 0)
        return USART;
    if (strcasecmp(name, UartH.name()) == 0)
        return UartH;
    return Devs::nullDevice();
}

void DevsTlcs90::enableDevice(Device &dev) {
    USART.enable(&dev == &USART);
    UartH.enable(&dev == &UartH);
}

void DevsTlcs90::printDevices() const {
    printDevice(USART);
    printDevice(UartH);
}

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
