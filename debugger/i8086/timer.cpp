#include "timer.h"
#include "debugger.h"

namespace debugger {
namespace i8086 {

namespace {
constexpr uint8_t CTRL = 0;
constexpr uint8_t TYPE = 1;
constexpr uint8_t EOI = 2;
constexpr uint8_t RUN = 0x01;
constexpr uint8_t PENDING = 0x80;
}  // namespace

void Timer::print() const {
    cli.print(_run ? "running" : "stopped");
    cli.print(", type ");
    cli.printHex(_type, 2);
    if (_pending)
        cli.print(", tick pending");
    cli.println();
}

void Timer::reset() {
    acknowledge();
    _run = false;
}

// Ticks keep their period across the slow service's jitter; after a
// pause longer than a period, such as a halt, they start afresh.
bool Timer::loop() {
    if (!_enabled)
        return false;
    if (_run) {
        const auto now = micros();
        if (int32_t(now - _due) >= 0) {
            _due += PERIOD_US;
            if (int32_t(now - _due) >= 0)
                _due = now + PERIOD_US;
            if (!_pending) {
                _pending = true;
                Debugger.target().assertInt(_type);
            }
        }
    }
    return true;
}

void Timer::acknowledge() {
    if (_pending) {
        _pending = false;
        Debugger.target().negateInt(_type);
    }
}

uint16_t Timer::peek(uint32_t addr) const {
    switch (addr - _base_addr) {
    case CTRL:
        return (_run ? RUN : 0) | (_pending ? PENDING : 0);
    case TYPE:
        return _type;
    default:
        return 0;
    }
}

void Timer::write(uint32_t addr, uint16_t data) {
    switch (addr - _base_addr) {
    case CTRL: {
        const auto run = (data & RUN) != 0;
        if (run && !_run)
            _due = micros() + PERIOD_US;
        if (!run)
            acknowledge();
        _run = run;
        break;
    }
    case TYPE: {
        const auto pending = _pending;
        acknowledge();
        _type = data;
        if (pending) {
            _pending = true;
            Debugger.target().assertInt(_type);
        }
        break;
    }
    case EOI:
        acknowledge();
        break;
    }
}

}  // namespace i8086
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
