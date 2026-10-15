#include "devs_cp1610.h"
#include <strings.h>
#include "i8251.h"

namespace debugger {
namespace cp1600 {

DevsCp1610::DevsCp1610() : _usart(new I8251()) {
    _usart->setBaseAddr(USART_BASE);
}

DevsCp1610::~DevsCp1610() {
    delete _usart;
}

void DevsCp1610::reset() {
    _usart->reset();
}

void DevsCp1610::begin() {
    enableDevice(_usart);
}

void DevsCp1610::loop() {
    _usart->loop();
}

bool DevsCp1610::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsCp1610::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsCp1610::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

uint16_t DevsCp1610::vector() const {
    return _usart->vector();
}

Device *DevsCp1610::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    return Devs::nullDevice();
}

void DevsCp1610::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
}

void DevsCp1610::printDevices() const {
    printDevice(_usart);
}

}  // namespace cp1600
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
