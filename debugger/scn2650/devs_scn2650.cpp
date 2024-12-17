#include "devs_scn2650.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"

namespace debugger {
namespace scn2650 {

DevsScn2650::DevsScn2650() : _usart(new I8251()) {}

DevsScn2650::~DevsScn2650() {
    delete _usart;
}

void DevsScn2650::reset() {
    _usart->reset();
    _usart->setBaseAddr(USART_BASE);
}

void DevsScn2650::begin() {
    enableDevice(_usart);
}

void DevsScn2650::loop() {
    _usart->loop();
}

bool DevsScn2650::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsScn2650::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsScn2650::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

uint16_t DevsScn2650::vector() const {
    return _usart->vector();
}

Device *DevsScn2650::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    return Devs::nullDevice();
}

void DevsScn2650::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
}

void DevsScn2650::printDevices() const {
    printDevice(_usart);
}

}  // namespace scn2650
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
