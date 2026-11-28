#include "tick.h"
#include "debugger.h"

namespace debugger {

namespace {
constexpr uint8_t STATUS = 0;
constexpr uint8_t INTR = 1;
constexpr uint8_t RATE = 2;

constexpr uint8_t PENDING = 0x80;
constexpr uint8_t RUN = 0x01;
}  // namespace

void Tick::print() const {
    const auto radix = Debugger.target().inputRadix();
    cli.print("  #IRQ: ");
    cli.printNum(_intr, radix, Debugger::numDigits(8, radix));
    cli.print(" rate: ");
    cli.printDec(_rate);
    cli.println(_run ? " Hz, running" : " Hz, stopped");
}

void Tick::reset() {
    acknowledge();
    _intr = 0;
    _rate = RATE_DEFAULT;
    _run = false;
}

// Ticks keep their period from one to the next; after a pause longer
// than a period, such as a halt, they start afresh.
bool Tick::loop() {
    if (!_enabled)
        return false;
    if (_run) {
        const auto now = micros();
        if (int32_t(now - _due) >= 0) {
            _due += period();
            if (int32_t(now - _due) >= 0)
                _due = now + period();
            if (!_pending) {
                _pending = true;
                if (_intr)
                    Debugger.target().assertInt(_intr);
            }
        }
    }
    return true;
}

void Tick::acknowledge() {
    if (_pending) {
        _pending = false;
        if (_intr)
            Debugger.target().negateInt(_intr);
    }
}

uint16_t Tick::peek(uint32_t addr) const {
    switch (addr - _base_addr) {
    case STATUS:
        return (_pending ? PENDING : 0) | (_run ? RUN : 0);
    case INTR:
        return _intr;
    case RATE:
        return _rate;
    default:
        return 0;
    }
}

uint16_t Tick::read(uint32_t addr) {
    const auto data = peek(addr);
    if (addr - _base_addr == STATUS)
        acknowledge();
    return data;
}

void Tick::write(uint32_t addr, uint16_t data) {
    switch (addr - _base_addr) {
    case STATUS: {
        const auto run = (data & RUN) != 0;
        if (run && !_run)
            _due = micros() + period();
        _run = run;
        acknowledge();
        break;
    }
    case INTR: {
        // A tick waiting on the old name moves to the new one.
        const auto pending = _pending;
        acknowledge();
        _intr = data;
        if (pending) {
            _pending = true;
            if (_intr)
                Debugger.target().assertInt(_intr);
        }
        break;
    }
    case RATE:
        if (data & 0xFF)
            _rate = data;
        break;
    }
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
