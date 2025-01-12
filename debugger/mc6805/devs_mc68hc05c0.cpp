#include "devs_mc68hc05c0.h"
#include <string.h>
#include "debugger.h"
#include "mc68hc05c0_sci_handler.h"

namespace debugger {
namespace mc68hc05c0 {

#if defined(ENABLE_SERIAL_HANDLER)

DevsMc68HC05C0::DevsMc68HC05C0(uint16_t acia_base)
    : DevsMc6805(acia_base), _sci(new Mc68HC05C0SciHandler()) {}

DevsMc68HC05C0::~DevsMc68HC05C0() {
    delete _sci;
}

void DevsMc68HC05C0::reset() {
    _sci->reset();
    DevsMc6805::reset();
}

void DevsMc68HC05C0::loop() {
    _sci->loop();
    DevsMc6805::loop();
}

void DevsMc68HC05C0::setIdle(bool idle) {
    _sci->setIdle(idle);
}

bool DevsMc68HC05C0::isSelected(uint32_t addr) const {
    return _sci->isSelected(addr) || DevsMc6805::isSelected(addr);
}

void DevsMc68HC05C0::write(uint32_t addr, uint16_t data) const {
    if (_sci->isSelected(addr)) {
        _sci->write(addr, data);
        return;
    }
    DevsMc6805::write(addr, data);
}

Device *DevsMc68HC05C0::parseDevice(const char *name) const {
    if (strcasecmp(name, _sci->name()) == 0)
        return _sci;
    return DevsMc6805::parseDevice(name);
}

void DevsMc68HC05C0::enableDevice(Device *dev) {
    _sci->enable(dev == _sci);
    DevsMc6805::enableDevice(dev);
}

void DevsMc68HC05C0::printDevices() const {
    DevsMc6805::printDevices();
    printDevice(_sci);
}

#endif

}  // namespace mc68hc05c0
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
