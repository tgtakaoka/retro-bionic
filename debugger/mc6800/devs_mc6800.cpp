#include "devs_mc6800.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"

namespace debugger {
namespace mc6800 {

DevsMc6800 Devices;

void DevsMc6800::reset() {
    ACIA.reset();
    ACIA.setBaseAddr(ACIA_BASE);
}

void DevsMc6800::begin() {
    enableDevice(ACIA);
}

void DevsMc6800::loop() {
    ACIA.loop();
}

bool DevsMc6800::isSelected(uint32_t addr) const {
    return ACIA.isSelected(addr);
}

uint16_t DevsMc6800::read(uint32_t addr) const {
    return ACIA.read(addr);
}

void DevsMc6800::write(uint32_t addr, uint16_t data) const {
    ACIA.write(addr, data);
}

Device &DevsMc6800::parseDevice(const char *name) const {
    if (strcasecmp(name, ACIA.name()) == 0)
        return ACIA;
    return Devs::nullDevice();
}

void DevsMc6800::enableDevice(Device &dev) {
    ACIA.enable(&dev == &ACIA);
}

void DevsMc6800::printDevices() const {
    printDevice(ACIA);
}

}  // namespace mc6800
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
