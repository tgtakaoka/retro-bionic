#include "devs_cdp1802.h"
#include <string.h>
#include "cdp1802_sci_handler.h"
#include "debugger.h"
#include "mc6850.h"

namespace debugger {
namespace cdp1802 {

Cdp1802SciHandler SciH;

DevsCdp1802 Devs;

void DevsCdp1802::reset() {
    ACIA.reset();
    SciH.reset();
    ACIA.setBaseAddr(ACIA_BASE);
}

void DevsCdp1802::begin() {
    enableDevice(ACIA);
}

void DevsCdp1802::loop() {
    ACIA.loop();
}

bool DevsCdp1802::isSelected(uint32_t addr) const {
    return ACIA.isSelected(addr);
}

uint16_t DevsCdp1802::read(uint32_t addr) const {
    return ACIA.read(addr);
}

void DevsCdp1802::write(uint32_t addr, uint16_t data) const {
    ACIA.write(addr, data);
}

Device &DevsCdp1802::parseDevice(const char *name) const {
    if (strcasecmp(name, ACIA.name()) == 0)
        return ACIA;
    if (strcasecmp(name, SciH.name()) == 0)
        return SciH;
    return Devs::nullDevice();
}

void DevsCdp1802::enableDevice(Device &dev) {
    ACIA.enable(&dev == &ACIA);
    SciH.enable(&dev == &SciH);
}

void DevsCdp1802::printDevices() const {
    printDevice(ACIA);
    printDevice(SciH);
}

}  // namespace cdp1802
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
