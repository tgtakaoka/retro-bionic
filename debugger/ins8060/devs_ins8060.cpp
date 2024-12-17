#include "devs_ins8060.h"
#include <string.h>
#include "debugger.h"
#include "ins8060_sci_handler.h"
#include "mc6850.h"

namespace debugger {
namespace ins8060 {

DevsIns8060::DevsIns8060()
    : _acia(new Mc6850()), _sci(new Ins8060SciHandler()) {}

DevsIns8060::~DevsIns8060() {
    delete _acia;
    delete _sci;
}

void DevsIns8060::reset() {
    _sci->reset();
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsIns8060::begin() {
    enableDevice(_acia);
}

void DevsIns8060::loop() {
    _acia->loop();
}

void DevsIns8060::setIdle(bool idle) {
    _sci->setIdle(idle);
}

bool DevsIns8060::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsIns8060::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsIns8060::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsIns8060::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
    if (strcasecmp(name, _sci->name()) == 0)
        return _sci;
    return Devs::nullDevice();
}

void DevsIns8060::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
    _sci->enable(dev == _sci);
}

void DevsIns8060::printDevices() const {
    printDevice(_acia);
    printDevice(_sci);
}

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
