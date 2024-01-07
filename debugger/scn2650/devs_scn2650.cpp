#include "devs_scn2650.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"

namespace debugger {
namespace scn2650 {

struct DevsScn2650 Devs;

void DevsScn2650::reset() {
    USART.reset();
    USART.setBaseAddr(USART_BASE);
}

void DevsScn2650::begin() {
    enableDevice(USART);
}

void DevsScn2650::loop() {
    USART.loop();
}

bool DevsScn2650::isSelected(uint32_t addr) const {
    return USART.isSelected(addr);
}

uint16_t DevsScn2650::read(uint32_t addr) const {
    return USART.read(addr);
}

void DevsScn2650::write(uint32_t addr, uint16_t data) const {
    USART.write(addr, data);
}

uint16_t DevsScn2650::vector() const {
    return USART.vector();
}

Device &DevsScn2650::parseDevice(const char *name) const {
    if (strcasecmp(name, USART.name()) == 0)
        return USART;
    return Devs::nullDevice();
}

void DevsScn2650::enableDevice(Device &dev) {
    USART.enable(&dev == &USART);
}

void DevsScn2650::printDevices() const {
    printDevice(USART);
}

}  // namespace scn2650
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
