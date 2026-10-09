#include "signals.h"
#if defined(ARDUINO)
#include <Arduino.h>
#include "watchdog.h"
#endif

namespace debugger {

namespace {
constexpr auto hold_delay_ns = 10;
}

Cycles *Cycles::_current = nullptr;

uint_fast8_t Cycles::_put = 0;
uint_fast8_t Cycles::_get = 0;
uint_fast8_t Cycles::_cycles = 0;
bool Cycles::_hold = false;
SignalsImpl Cycles::_ring[MAX_CYCLES];

Cycles::Cycles() {
    _current = this;
    reset();
}

Cycles::~Cycles() {
    if (_current == this)
        _current = nullptr;
}

SignalsImpl *Cycles::head() {
    return &_ring[_put];
}

SignalsImpl *Cycles::tail() {
    return &_ring[_get];
}

SignalsImpl *Cycles::at(uint_fast8_t index) {
    return &_ring[index % MAX_CYCLES];
}

uint_fast8_t Cycles::indexOf(const SignalsImpl *s) {
    return s - _ring;
}

void Cycles::reset() {
    _cycles = 0;
    _ring[_get = _put = 0].clear();
}

void Cycles::next() {
    if (!_hold) {
        _put = (_put + 1) % MAX_CYCLES;
        // The head slot is the transaction in progress, so the ring holds
        // one fewer completed cycles than it has slots. Letting the count
        // reach MAX_CYCLES left _get on the head for one cycle: with
        // exactly that many recorded, tail and head coincided and the dump
        // was empty.
        if (_cycles < MAX_CYCLES - 1) {
            _cycles++;
        } else {
            _get = (_put + 1) % MAX_CYCLES;
        }
    } else {
#if defined(ARDUINO)
        // As long as an advance, so a bus cycle's timing does not change
        // while the ring is held.
        delayNanoseconds(hold_delay_ns);
#endif
    }
    _ring[_put].clear();
#if defined(ARDUINO)
    Watchdog::tick();
#endif
}

void Cycles::discard(const SignalsImpl *s) {
    const auto drop = s->diff(head());
    if (_cycles < drop) {
        _cycles = 0;
        _put = _get;
    } else {
        _cycles -= drop;
        _put = s->pos();
    }
    _ring[_put].clear();
}

void Cycles::dispose(const SignalsImpl *s) {
    const auto drop = tail()->diff(s);
    _cycles -= drop;
    _get = s->pos();
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
