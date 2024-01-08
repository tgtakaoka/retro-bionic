#include "devs_i8085.h"
#include <string.h>
#include "debugger.h"
#include "i8085_sio_handler.h"
#include "i8251.h"

namespace debugger {
namespace i8085 {

I8085SioHandler SioH;

DevsI8085 Devs;

void DevsI8085::reset() {
    SioH.reset();
    USART.reset();
    USART.setBaseAddr(USART_BASE);
}

void DevsI8085::begin() {
    enableDevice(USART);
}

void DevsI8085::loop() {
    USART.loop();
}

bool DevsI8085::isSelected(uint32_t addr) const {
    return USART.isSelected(addr);
}

uint16_t DevsI8085::read(uint32_t addr) const {
    return USART.read(addr);
}

void DevsI8085::write(uint32_t addr, uint16_t data) const {
    USART.write(addr, data);
}

Device &DevsI8085::parseDevice(const char *name) const {
    if (strcasecmp(name, USART.name()) == 0)
        return USART;
    if (strcasecmp(name, SioH.name()) == 0)
        return SioH;
    return Devs::nullDevice();
}

void DevsI8085::enableDevice(Device &dev) {
    USART.enable(&dev == &USART);
    SioH.enable(&dev == &SioH);
}

void DevsI8085::printDevices() const {
    printDevice(USART);
    printDevice(SioH);
}

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
