#include "devs_i8048.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"

namespace debugger {
namespace i8048 {

DevsI8048 Devs;

void DevsI8048::reset() {
    USART.reset();
    USART.setBaseAddr(USART_BASE);
}

void DevsI8048::begin() {
    enableDevice(USART);
}

void DevsI8048::loop() {
    USART.loop();
}

bool DevsI8048::isSelected(uint32_t addr) const {
    return USART.isSelected(addr);
}

uint16_t DevsI8048::read(uint32_t addr) const {
    return USART.read(addr);
}

void DevsI8048::write(uint32_t addr, uint16_t data) const {
    USART.write(addr, data);
}

Device &DevsI8048::parseDevice(const char *name) const {
    if (strcasecmp(name, USART.name()) == 0)
        return USART;
    return Devs::nullDevice();
}

void DevsI8048::enableDevice(Device &dev) {
    USART.enable(&dev == &USART);
}

void DevsI8048::printDevices() const {
    printDevice(USART);
}

}  // namespace i8048
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
