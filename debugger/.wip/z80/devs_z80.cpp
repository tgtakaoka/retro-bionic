#include "devs_z80.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"

namespace debugger {
namespace z80 {

DevsZ80 Devs;

void DevsZ80::reset() {
    USART.reset();
    USART.setBaseAddr(USART_BASE);
}

void DevsZ80::begin() {
    enableDevice(USART);
}

void DevsZ80::loop() {
    USART.loop();
}

bool DevsZ80::isSelected(uint32_t addr) const {
    return USART.isSelected(addr);
}

uint16_t DevsZ80::read(uint32_t addr) const {
    return USART.read(addr);
}

void DevsZ80::write(uint32_t addr, uint16_t data) const {
    USART.write(addr, data);
}

uint16_t DevsZ80::vector() const {
    return USART.vector();
}

Device &DevsZ80::parseDevice(const char *name) const {
    if (strcasecmp(name, USART.name()) == 0)
        return USART;
    return Devs::nullDevice();
}

void DevsZ80::enableDevice(Device &dev) {
    USART.enable(&dev == &USART);
}

void DevsZ80::printDevices() const {
    printDevice(USART);
}

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
