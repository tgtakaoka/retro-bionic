#include "devs_i8085.h"
#include <string.h>
#include "debugger.h"
#include "i8085_sio_handler.h"
#include "i8251.h"

namespace debugger {
namespace i8085 {

DevsI8085::DevsI8085() : _usart(new I8251()), _sio(new I8085SioHandler()) {}

DevsI8085::~DevsI8085() {
    delete _usart;
    delete _sio;
}

void DevsI8085::reset() {
    _sio->reset();
    _usart->reset();
    _usart->setBaseAddr(USART_BASE);
}

void DevsI8085::begin() {
    enableDevice(_usart);
}

void DevsI8085::loop() {
    _usart->loop();
}

void DevsI8085::setIdle(bool idle) {
    _sio->setIdle(idle);
}

bool DevsI8085::isSelected(uint32_t addr) const {
    return _usart->isSelected(addr);
}

uint16_t DevsI8085::read(uint32_t addr) const {
    return _usart->read(addr);
}

void DevsI8085::write(uint32_t addr, uint16_t data) const {
    _usart->write(addr, data);
}

uint16_t DevsI8085::vector() const {
    return _usart->vector();
}

Device *DevsI8085::parseDevice(const char *name) const {
    if (strcasecmp(name, _usart->name()) == 0)
        return _usart;
    if (strcasecmp(name, _sio->name()) == 0)
        return _sio;
    return Devs::nullDevice();
}

void DevsI8085::enableDevice(Device *dev) {
    _usart->enable(dev == _usart);
    _sio->enable(dev == _sio);
}

void DevsI8085::printDevices() const {
    printDevice(_usart);
    printDevice(_sio);
}

}  // namespace i8085
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
