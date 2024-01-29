#include "devs_mc68hc11.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"
#include "mc68hc11_init.h"
#include "mc68hc11_sci_handler.h"
#include "regs_mc68hc11.h"

namespace debugger {
namespace mc68hc11 {

void DevsMc68hc11::reset() {
    _init->reset();
    _sci->reset();
    ACIA.reset();
    ACIA.setBaseAddr(ACIA_BASE);
}

void DevsMc68hc11::begin() {
    enableDevice(ACIA);
}

void DevsMc68hc11::loop() {
    ACIA.loop();
    _sci->loop();
}

bool DevsMc68hc11::isSelected(uint32_t addr) const {
    return ACIA.isSelected(addr) || _sci->isSelected(addr);
}

uint16_t DevsMc68hc11::read(uint32_t addr) const {
    return ACIA.isSelected(addr) ? ACIA.read(addr) : _sci->read(addr);
}

void DevsMc68hc11::write(uint32_t addr, uint16_t data) const {
    if (ACIA.isSelected(addr)) {
        ACIA.write(addr, data);
    } else {
        _sci->write(addr, data);
    }
}

Device &DevsMc68hc11::parseDevice(const char *name) const {
    if (strcasecmp(name, ACIA.name()) == 0)
        return ACIA;
    if (strcasecmp(name, _sci->name()) == 0)
        return *_sci;
    if (strcasecmp(name, _init->name()) == 0)
        return *_init;
    return Devs::nullDevice();
}

void DevsMc68hc11::enableDevice(Device &dev) {
    if (&dev == _init)
        return;
    ACIA.enable(&dev == &ACIA);
    _sci->enable(&dev == _sci);
}

void DevsMc68hc11::printDevices() const {
    printDevice(ACIA);
    printDevice(*_sci);
    printDevice(*_init);
}

}  // namespace mc68hc11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
