#include "devs_mc68hc05c0.h"
#include <string.h>
#include "debugger.h"
#include "mc68hc05c0_sci_handler.h"

namespace debugger {
namespace mc68hc05c0 {

struct Mc68HC05C0SciHandler SciH;

void DevsMc68HC05C0::reset() {
    SciH.reset();
    DevsMc6805::reset();
}

void DevsMc68HC05C0::loop() {
    SciH.loop();
    DevsMc6805::loop();
}

bool DevsMc68HC05C0::isSelected(uint32_t addr) const {
    return DevsMc6805::isSelected(addr) || SciH.isSelected(addr);
}

uint16_t DevsMc68HC05C0::read(uint32_t addr) const {
    return DevsMc6805::isSelected(addr) ? DevsMc6805::read(addr)
                                        : SciH.read(addr);
}

void DevsMc68HC05C0::write(uint32_t addr, uint16_t data) const {
    if (DevsMc6805::isSelected(addr)) {
        DevsMc6805::write(addr, data);
    } else {
        SciH.write(addr, data);
    }
}

Device &DevsMc68HC05C0::parseDevice(const char *name) const {
    if (strcasecmp(name, SciH.name()) == 0)
        return SciH;
    return DevsMc6805::parseDevice(name);
}

void DevsMc68HC05C0::enableDevice(Device &dev) {
    SciH.enable(&dev == &SciH);
    DevsMc6805::enableDevice(dev);
}

void DevsMc68HC05C0::printDevices() const {
    DevsMc6805::printDevices();
    printDevice(SciH);
}

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
