#include "devs_mc68hc11.h"
#include <string.h>
#include "debugger.h"
#include "mc6850.h"
#include "mc68hc11_init.h"
#include "mc68hc11_sci_handler.h"
#include "regs_mc68hc11.h"

namespace debugger {
namespace mc68hc11 {

DevsMc68hc11::DevsMc68hc11(Mc68hc11Init &init)
    : _init(init), _sci(new Mc68hc11SciHandler(init)), _acia(new Mc6850()) {}

DevsMc68hc11::~DevsMc68hc11() {
    delete _sci;
    delete _acia;
}

void DevsMc68hc11::reset() {
    _init.reset();
    _sci->reset();
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsMc68hc11::begin() {
    enableDevice(_acia);
}

void DevsMc68hc11::loop() {
    _acia->loop();
    _sci->loop();
}

void DevsMc68hc11::setIdle(bool idle) {
    _sci->setIdle(idle);
}

bool DevsMc68hc11::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr) || _sci->isSelected(addr);
}

uint16_t DevsMc68hc11::read(uint32_t addr) const {
    return _acia->isSelected(addr) ? _acia->read(addr) : _sci->read(addr);
}

void DevsMc68hc11::write(uint32_t addr, uint16_t data) const {
    if (_acia->isSelected(addr)) {
        _acia->write(addr, data);
    } else {
        _sci->write(addr, data);
    }
}

Device *DevsMc68hc11::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    if (strcasecmp(name, _sci->name()) == 0)
        return _sci;
    if (strcasecmp(name, _init.name()) == 0)
        return &_init;
    return Devs::nullDevice();
}

void DevsMc68hc11::enableDevice(Device *dev) {
    if (dev == &_init)
        return;
    _acia->enable(dev == _acia);
    _sci->enable(dev == _sci);
}

void DevsMc68hc11::printDevices() const {
    printDevice(_acia);
    printDevice(_sci);
    printDevice(&_init);
}

}  // namespace mc68hc11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
