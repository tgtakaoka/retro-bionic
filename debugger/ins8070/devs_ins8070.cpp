#include "devs_ins8070.h"
#include <string.h>
#include "mc6850.h"

namespace debugger {
namespace ins8070 {

DevsIns8070::DevsIns8070()
    : _acia(new Mc6850())
#if defined(ENABLE_SERIAL_HANDLER)
      ,
      _sci(new Ins8070SciHandler())
#endif
{
}

DevsIns8070::~DevsIns8070() {
    delete _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    delete _sci;
#endif
}

void DevsIns8070::reset() {
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->reset();
#endif
    _acia->reset();
    _acia->setBaseAddr(ACIA_BASE);
}

void DevsIns8070::begin() {
    enableDevice(_acia);
}

void DevsIns8070::loop() {
    _acia->loop();
}

void DevsIns8070::setIdle(bool idle) {
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->setIdle(idle);
#endif
}

bool DevsIns8070::isSelected(uint32_t addr) const {
    return _acia->isSelected(addr);
}

uint16_t DevsIns8070::read(uint32_t addr) const {
    return _acia->read(addr);
}

void DevsIns8070::write(uint32_t addr, uint16_t data) const {
    _acia->write(addr, data);
}

Device *DevsIns8070::parseDevice(const char *name) const {
    if (strcasecmp(name, _acia->name()) == 0)
        return _acia;
#if defined(ENABLE_SERIAL_HANDLER)
    if (strcasecmp(name, _sci->name()) == 0)
        return _sci;
#endif
    return Devs::nullDevice();
}

void DevsIns8070::enableDevice(Device *dev) {
    _acia->enable(dev == _acia);
#if defined(ENABLE_SERIAL_HANDLER)
    _sci->enable(dev == _sci);
#endif
}

void DevsIns8070::printDevices() const {
    printDevice(_acia);
#if defined(ENABLE_SERIAL_HANDLER)
    printDevice(_sci);
#endif
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
