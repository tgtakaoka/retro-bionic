#include "devs_d8087.h"
#include <strings.h>
#include "debugger.h"
#include "pins_i8086.h"

namespace debugger {
namespace i8086 {

void NpxInt::print() const {
    const auto radix = Debugger.target().inputRadix();
    cli.print("  INT: ");
    cli.printNum(_type, radix, Debugger::numDigits(8, radix));
    cli.println(_type == InstI8086::VEC_NMI ? " NMI" : " INTR");
}

// digitalReadFast() may return the port bit, not 1: compare with LOW.
void NpxInt::loop() {
    if (!_enabled || _type == 0)
        return;
    const auto high = digitalReadFast(PIN_INT) != LOW;
    if (high && !_asserted) {
        _asserted = true;
        Debugger.target().assertInt(_type);
    } else if (!high) {
        negate();
    }
}

void NpxInt::enable(bool enabled) {
    if (!enabled)
        negate();
    Device::enable(enabled);
}

void NpxInt::setVector(uint8_t type, uint8_t) {
    negate();
    _type = type;
}

uint16_t NpxInt::vector() const {
    return (_asserted && _type != InstI8086::VEC_NMI) ? _type : 0;
}

void NpxInt::negate() {
    if (_asserted) {
        _asserted = false;
        Debugger.target().negateInt(_type);
    }
}

DevsD8087::DevsD8087() : DevsI8080(), _npx(new NpxInt()) {}

DevsD8087::~DevsD8087() {
    delete _npx;
}

void DevsD8087::reset() {
    _npx->reset();
    DevsI8080::reset();
}

void DevsD8087::loop() {
    DevsI8080::loop();
    _npx->loop();
}

uint16_t DevsD8087::vector() const {
    const auto type = _npx->vector();
    return type ? type : DevsI8080::vector();
}

Device *DevsD8087::parseDevice(const char *name) const {
    if (strcasecmp(name, _npx->name()) == 0)
        return _npx;
    return DevsI8080::parseDevice(name);
}

// NPX goes on and off by itself; the console stays as it is.
void DevsD8087::enableDevice(Device *dev) {
    if (dev == _npx) {
        _npx->enable(!_npx->isEnabled());
        return;
    }
    DevsI8080::enableDevice(dev);
}

void DevsD8087::printDevices() const {
    DevsI8080::printDevices();
    printDevice(_npx);
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
