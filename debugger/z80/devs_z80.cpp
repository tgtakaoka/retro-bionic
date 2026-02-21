#include "devs_z80.h"
#include <string.h>
#include "i8251.h"

namespace debugger {
namespace z80 {

DevsZ80::DevsZ80() : _usart(new I8251()) {}

DevsZ80::~DevsZ80() {
    delete _usart;
}

void DevsZ80::reset() {
    _usart->reset();
    _usart->setBaseAddr(USART_BASE);
}

void DevsZ80::begin() {
    enableDevice(_usart);
}

void DevsZ80::loop() {
    _usart->loop();
}

bool DevsZ80::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsZ80::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsZ80::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

uint16_t DevsZ80::vector() const {
    return _usart->vector();
}

Device *DevsZ80::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    return Devs::nullDevice();
}

void DevsZ80::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
}

void DevsZ80::printDevices() const {
    printDevice(_usart);
}

}  // namespace z80
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
