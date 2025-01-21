#include "devs_mn1613.h"
#include <string.h>
#include "debugger.h"
#include "i8251.h"

namespace debugger {
namespace mn1613 {

DevsMn1613::DevsMn1613() : _usart(new I8251()) {}

DevsMn1613::~DevsMn1613() {
    delete _usart;
}

void DevsMn1613::reset() {
    _usart->reset();
    _usart->setBaseAddr(USART_BASE);
}

void DevsMn1613::begin() {
    enableDevice(_usart);
}

void DevsMn1613::loop() {
    _usart->loop();
}

bool DevsMn1613::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsMn1613::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsMn1613::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

Device *DevsMn1613::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    return Devs::nullDevice();
}

void DevsMn1613::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
}

void DevsMn1613::printDevices() const {
    printDevice(_usart);
}

}  // namespace mn1613
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
