#include "devs_ins8070.h"
#include <string.h>
#include "debugger.h"
#include "ins8070_sci_handler.h"
#include "mc6850.h"

namespace debugger {
namespace ins8070 {

Ins8070SciHandler SciH;

DevsIns8070 Devs;

void DevsIns8070::reset() {
    SciH.reset();
    ACIA.reset();
    ACIA.setBaseAddr(ACIA_BASE);
}

void DevsIns8070::begin() {
    enableDevice(ACIA);
}

void DevsIns8070::loop() {
    ACIA.loop();
}

void DevsIns8070::setIdle(bool idle) {
    SciH.setIdle(idle);
}

bool DevsIns8070::isSelected(uint32_t addr) const {
    return ACIA.isSelected(addr);
}

uint16_t DevsIns8070::read(uint32_t addr) const {
    return ACIA.read(addr);
}

void DevsIns8070::write(uint32_t addr, uint16_t data) const {
    ACIA.write(addr, data);
}

Device &DevsIns8070::parseDevice(const char *name) const {
    if (strcasecmp(name, ACIA.name()) == 0)
        return ACIA;
    if (strcasecmp(name, SciH.name()) == 0)
        return SciH;
    return Devs::nullDevice();
}

void DevsIns8070::enableDevice(Device &dev) {
    ACIA.enable(&dev == &ACIA);
    SciH.enable(&dev == &SciH);
}

void DevsIns8070::printDevices() const {
    printDevice(ACIA);
    printDevice(SciH);
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
