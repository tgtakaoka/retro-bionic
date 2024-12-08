#include "debugger.h"

#include "pins.h"

namespace debugger {

void Pins::reset() {
#ifdef PIN_DEBUG
    negate_debug();
    pinMode(PIN_DEBUG, OUTPUT);
#endif
    pinMode(PIN_USRSW, INPUT_PULLUP);
#ifdef PIN_USRLED
    pinMode(PIN_USRLED, OUTPUT);
#endif
    setHalt();
    resetPins();
}

void Pins::setRun() const {
#ifdef PIN_USRLED
    digitalWriteFast(PIN_USRLED, LOW);
#endif
}

void Pins::setHalt() const {
#ifdef PIN_USRLED
    digitalWriteFast(PIN_USRLED, HIGH);
#endif
}

bool Pins::haltSwitch() {
    return digitalReadFast(PIN_USRSW) == LOW;
}

void Pins::pinsMode(const uint8_t *pins, uint8_t size, uint8_t mode) {
    for (auto i = 0; i < size; ++i)
        pinMode(pins[i], mode);
}

void Pins::pinsMode(
        const uint8_t *pins, uint8_t size, uint8_t mode, uint8_t val) {
    for (auto i = 0; i < size; ++i) {
        const auto pin = pins[i];
        // This order is critical to initialize OUTPUT/HIGH pins.
        pinMode(pin, mode);
        digitalWrite(pin, val);
    }
}

void Pins::assert_debug() {
#ifdef PIN_DEBUG
    digitalWriteFast(PIN_DEBUG, HIGH);
#endif
}

void Pins::negate_debug() {
#ifdef PIN_DEBUG
    digitalWriteFast(PIN_DEBUG, LOW);
#endif
}

void Pins::toggle_debug() {
#ifdef PIN_DEBUG
    digitalToggleFast(PIN_DEBUG);
#endif
}

bool Pins::isBreakPoint(uint32_t addr) const {
    return Debugger.isBreakPoint(addr);
}

void Pins::saveBreakInsts() const {
    Debugger.saveBreakInsts();
}

void Pins::restoreBreakInsts() const {
    Debugger.restoreBreakInsts();
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
