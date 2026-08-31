#include "signals.h"

namespace debugger {

uint_fast8_t Cycles::_put = 0;
uint_fast8_t Cycles::_get = 0;
uint_fast8_t Cycles::_cycles = 0;
SignalsImpl Cycles::_ring[MAX_CYCLES];

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
    _put = (_put + 1) % MAX_CYCLES;
    if (_cycles < MAX_CYCLES) {
        _cycles++;
    } else {
        _get = (_put + 1) % MAX_CYCLES;
    }
    _ring[_put].clear();
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

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
