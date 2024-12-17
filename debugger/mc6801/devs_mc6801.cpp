#include "devs_mc6801.h"
#include <string.h>
#include "debugger.h"
#include "mc6801_sci_handler.h"
#include "mc6850.h"

namespace debugger {
namespace mc6801 {

DevsMc6801::DevsMc6801() : _acia(new Mc6850()), _sci(new Mc6801SciHandler()) {}

DevsMc6801::~DevsMc6801() {
    delete _acia;
    delete _sci;
}

void DevsMc6801::reset() {
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
    _sci->reset();
}

void DevsMc6801::begin() {
    enableDevice(_acia);
}

void DevsMc6801::loop() {
    _acia->loop();
    _sci->loop();
}

void DevsMc6801::setIdle(bool idle) {
    _sci->setIdle(idle);
}

bool DevsMc6801::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr) || _sci->isSelected(addr);
}

uint16_t DevsMc6801::read(uint32_t addr) const {
    return _acia->isSelected(addr) ? _acia->read(addr) : _sci->read(addr);
}

void DevsMc6801::write(uint32_t addr, uint16_t data) const {
    if (_acia->isSelected(addr)) {
        _acia->write(addr, data);
    } else {
        _sci->write(addr, data);
    }
}

Device *DevsMc6801::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    if (strcasecmp(name, _sci->name()) == 0)
        return _sci;
    return Devs::nullDevice();
}

void DevsMc6801::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
    _sci->enable(dev == _sci);
}

void DevsMc6801::printDevices() const {
    printDevice(_acia);
    printDevice(_sci);
}

}  // namespace mc6801
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
