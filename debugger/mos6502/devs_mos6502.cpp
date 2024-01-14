#include "devs_mos6502.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"

namespace debugger {
namespace mos6502 {

DevsMos6502 Devices;

void DevsMos6502::reset() {
    ACIA.reset();
    ACIA.setBaseAddr(ACIA_BASE);
}

void DevsMos6502::begin() {
    enableDevice(ACIA);
}

void DevsMos6502::loop() {
    ACIA.loop();
}

bool DevsMos6502::isSelected(uint32_t addr) const {
    return ACIA.isSelected(addr);
}

uint16_t DevsMos6502::read(uint32_t addr) const {
    return ACIA.read(addr);
}

void DevsMos6502::write(uint32_t addr, uint16_t data) const {
    ACIA.write(addr, data);
}

Device &DevsMos6502::parseDevice(const char *name) const {
    if (strcasecmp(name, ACIA.name()) == 0)
        return ACIA;
    return Devs::nullDevice();
}

void DevsMos6502::enableDevice(Device &dev) {
    ACIA.enable(&dev == &ACIA);
}

void DevsMos6502::printDevices() const {
    printDevice(ACIA);
}

}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
