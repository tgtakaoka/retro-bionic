#include "debugger.h"

#include "pins.h"

namespace debugger {

Pins::Pins() : _breakNum(0) {}

void Pins::resetPins() {
#ifdef PIN_DEBUG
    negate_debug();
    pinMode(PIN_DEBUG, OUTPUT);
#endif
    reset();
    pinMode(PIN_USRSW, INPUT_PULLUP);
    pinMode(PIN_USRLED, OUTPUT);
    Debugger.devs().reset();
    setHalt();
}

void Pins::setRun() const {
    digitalWriteFast(PIN_USRLED, LOW);
}

void Pins::setHalt() const {
    digitalWriteFast(PIN_USRLED, HIGH);
}

bool Pins::haltSwitch() {
    return digitalReadFast(PIN_USRSW) == LOW;
}

bool Pins::setBreakPoint(uint32_t addr) {
    auto i = 0;
    while (i < _breakNum) {
        if (_breakPoints[i] == addr)
            return true;
        ++i;
    }
    if (i < BREAK_POINTS) {
        _breakPoints[i] = addr;
        ++_breakNum;
        return true;
    }
    return false;
}

bool Pins::clearBreakPoint(uint8_t index) {
    if (--index >= _breakNum)
        return false;
    for (auto i = index + 1; i < _breakNum; ++i) {
        _breakPoints[index] = _breakPoints[i];
        ++index;
    }
    --_breakNum;
    return true;
}

bool Pins::printBreakPoints() const {
    cli.println();
    for (uint8_t i = 0; i < _breakNum; ++i) {
        cli.printDec(i + 1, -2);
        Debugger.mems().disassemble(_breakPoints[i], 1);
    }
    return _breakNum != 0;
}

void Pins::saveBreakInsts() {
    for (auto i = 0; i < _breakNum; ++i) {
        const auto addr = _breakPoints[i];
        _breakInsts[i] = Debugger.mems().get_inst(addr);
        setBreakInst(addr);
    }
}

void Pins::restoreBreakInsts() {
    for (auto i = 0; i < _breakNum; ++i) {
        Debugger.mems().put_inst(_breakPoints[i], _breakInsts[i]);
    }
}

bool Pins::isBreakPoint(uint32_t addr) const {
    for (auto i = 0; i < _breakNum; ++i) {
        if (_breakPoints[i] == addr)
            return true;
    }
    return false;
}

void Pins::pinsMode(const uint8_t *pins, uint8_t size, uint8_t mode) {
    for (uint8_t i = 0; i < size; ++i)
        pinMode(pins[i], mode);
}

void Pins::pinsMode(
        const uint8_t *pins, uint8_t size, uint8_t mode, uint8_t val) {
    for (uint8_t i = 0; i < size; ++i) {
        const auto pin = pins[i];
        digitalWrite(pin, val);
        pinMode(pin, mode);
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

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
