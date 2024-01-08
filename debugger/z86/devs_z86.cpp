#include "devs_z86.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"
#include "z86_sio_handler.h"

namespace debugger {
namespace z86 {

Z86SioHandler SioH;

DevsZ86 Devs;

void DevsZ86::reset() {
    SioH.reset();
    USART.reset();
    USART.setBaseAddr(USART_BASE);
}

void DevsZ86::begin() {
    enableDevice(USART);
}

void DevsZ86::loop() {
    USART.loop();
}

bool DevsZ86::isSelected(uint32_t addr) const {
    return USART.isSelected(addr);
}

uint16_t DevsZ86::read(uint32_t addr) const {
    return USART.read(addr);
}

void DevsZ86::write(uint32_t addr, uint16_t data) const {
    USART.write(addr, data);
}

Device &DevsZ86::parseDevice(const char *name) const {
    if (strcasecmp(name, USART.name()) == 0)
        return USART;
    if (strcasecmp(name, SioH.name()) == 0)
        return SioH;
    return Devs::nullDevice();
}

void DevsZ86::enableDevice(Device &dev) {
    USART.enable(&dev == &USART);
    SioH.enable(&dev == &SioH);
}

void DevsZ86::printDevices() const {
    printDevice(USART);
    printDevice(SioH);
}

}  // namespace z86
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
