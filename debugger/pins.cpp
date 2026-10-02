#include "debugger.h"
#include "watchdog.h"

namespace debugger {

Pins::~Pins() {
    delete _regs;
    delete _mems;
    delete _devs;
}

void Pins::reset() {
    initDebug();
    setHalt();
    resetPins();
}

void Pins::setRun() const {
    _halted = false;
#ifdef PIN_USRLED
    digitalWriteFast(PIN_USRLED, LOW);
#endif
}

void Pins::setHalt() const {
    _halted = false;
#ifdef PIN_USRLED
    digitalWriteFast(PIN_USRLED, HIGH);
#endif
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

volatile bool Pins::_halted;

void Pins::isrHaltSwitch() {
    _halted = true;
}

void Pins::initDebug() {
    pinMode(PIN_USRSW, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_USRSW), isrHaltSwitch, FALLING);
#ifdef PIN_DEBUG
    negate_debug();
    pinMode(PIN_DEBUG, OUTPUT);
#endif
#ifdef PIN_USRLED
    pinMode(PIN_USRLED, OUTPUT);
#endif
}

bool Pins::haltSwitch() {
    // The core polls the halt port only from yield(), and a run reaches
    // yield() only through console I/O (usb_serial_available() yields
    // when the buffer is empty), so a program that never enables the
    // console device could not be stopped. Poll here.
    yield();
    Watchdog::tick();
    return _halted;
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
    return Debugger.breakPoints().on(addr);
}

void Pins::saveBreakInsts() const {
    Debugger.breakPoints().saveInsts();
}

void Pins::restoreBreakInsts() const {
    Debugger.breakPoints().restoreInsts();
}

void Pins::startRunTimer() {
    _startMicros = micros();
    _runMicros = 0;
}

void Pins::stopRunTimer() {
    _runMicros = micros() - _startMicros;
}

void Pins::disassembleCycles() {
    if (_lineLimit == 0)
        return;  // nothing will print; finding where to start would too
    if (_lineLimit != UINT32_MAX)
        Cycles::dispose(findBacktraceStart());
    printBacktrace();
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
