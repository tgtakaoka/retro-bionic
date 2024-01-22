#include "devs_ins8060.h"
#include <string.h>
#include "debugger.h"
#include "ins8060_sci_handler.h"
#include "mc6850.h"

namespace debugger {
namespace ins8060 {

Ins8060SciHandler SciH;

DevsIns8060 Devs;

void DevsIns8060::reset() {
    SciH.reset();
    ACIA.reset();
    ACIA.setBaseAddr(ACIA_BASE);
}

void DevsIns8060::begin() {
    enableDevice(ACIA);
}

void DevsIns8060::loop() {
    ACIA.loop();
}

bool DevsIns8060::isSelected(uint32_t addr) const {
    return ACIA.isSelected(addr);
}

uint16_t DevsIns8060::read(uint32_t addr) const {
    return ACIA.read(addr);
}

void DevsIns8060::write(uint32_t addr, uint16_t data) const {
    ACIA.write(addr, data);
}

Device &DevsIns8060::parseDevice(const char *name) const {
    if (strcasecmp(name, ACIA.name()) == 0)
        return ACIA;
    if (strcasecmp(name, SciH.name()) == 0)
        return SciH;
    return Devs::nullDevice();
}

void DevsIns8060::enableDevice(Device &dev) {
    ACIA.enable(&dev == &ACIA);
    SciH.enable(&dev == &SciH);
}

void DevsIns8060::printDevices() const {
    printDevice(ACIA);
    printDevice(SciH);
}

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
