#include "devs_mc6809.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"

namespace debugger {
namespace mc6809 {

DevsMc6809 Devs;

void DevsMc6809::reset() {
    ACIA.reset();
    ACIA.setBaseAddr(ACIA_BASE);
}

void DevsMc6809::begin() {
    enableDevice(ACIA);
}

void DevsMc6809::loop() {
    ACIA.loop();
}

bool DevsMc6809::isSelected(uint32_t addr) const {
    return ACIA.isSelected(addr);
}

uint16_t DevsMc6809::read(uint32_t addr) const {
    return ACIA.read(addr);
}

void DevsMc6809::write(uint32_t addr, uint16_t data) const {
    ACIA.write(addr, data);
}

Device &DevsMc6809::parseDevice(const char *name) const {
    if (strcasecmp(name, ACIA.name()) == 0)
        return ACIA;
    return Devs::nullDevice();
}

void DevsMc6809::enableDevice(Device &dev) {
    ACIA.enable(&dev == &ACIA);
}

void DevsMc6809::printDevices() const {
    printDevice(ACIA);
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
