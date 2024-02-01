#include "devs_mc146805e2.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"
#include "pins_mc146805e2.h"

namespace debugger {
namespace mc146805e2 {

struct DevsMc146805E2 Devices(ACIA_BASE);

void DevsMc146805E2::reset() {
    ACIA.reset();
    ACIA.setBaseAddr(_acia);
}

void DevsMc146805E2::begin() {
    enableDevice(ACIA);
}

void DevsMc146805E2::loop() {
    ACIA.loop();
}

bool DevsMc146805E2::isSelected(uint32_t addr) const {
    return ACIA.isSelected(addr);
}

uint16_t DevsMc146805E2::read(uint32_t addr) const {
    return ACIA.read(addr);
}

void DevsMc146805E2::write(uint32_t addr, uint16_t data) const {
    ACIA.write(addr, data);
}

Device &DevsMc146805E2::parseDevice(const char *name) const {
    if (strcasecmp(name, ACIA.name()) == 0)
        return ACIA;
    return Devs::nullDevice();
}

void DevsMc146805E2::enableDevice(Device &dev) {
    ACIA.enable(&dev == &ACIA);
}

void DevsMc146805E2::printDevices() const {
    printDevice(ACIA);
}

}  // namespace mc146805e2
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
