#include "devs_mc6801.h"
#include <string.h>
#include "debugger.h"
#include "mc6801_sci_handler.h"
#include "mc6850.h"

namespace debugger {
namespace mc6801 {

struct DevsMc6801 Devices {
    &SciH
};

void DevsMc6801::reset() {
    ACIA.reset();
    ACIA.setBaseAddr(ACIA_BASE);
    _sci->reset();
}

void DevsMc6801::begin() {
    enableDevice(ACIA);
}

void DevsMc6801::loop() {
    ACIA.loop();
    _sci->loop();
}

bool DevsMc6801::isSelected(uint32_t addr) const {
    return ACIA.isSelected(addr) || _sci->isSelected(addr);
}

uint16_t DevsMc6801::read(uint32_t addr) const {
    return ACIA.isSelected(addr) ? ACIA.read(addr) : _sci->read(addr);
}

void DevsMc6801::write(uint32_t addr, uint16_t data) const {
    if (ACIA.isSelected(addr)) {
        ACIA.write(addr, data);
    } else {
        _sci->write(addr, data);
    }
}

Device &DevsMc6801::parseDevice(const char *name) const {
    if (strcasecmp(name, ACIA.name()) == 0)
        return ACIA;
    if (strcasecmp(name, _sci->name()) == 0)
        return SciH;
    return Devs::nullDevice();
}

void DevsMc6801::enableDevice(Device &dev) {
    ACIA.enable(&dev == &ACIA);
    _sci->enable(&dev == _sci);
}

void DevsMc6801::printDevices() const {
    printDevice(ACIA);
    printDevice(*_sci);
}

}  // namespace mc6801
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
