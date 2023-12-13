#include "signals.h"

namespace debugger {

uint8_t SignalsImpl::_put = 0;
uint8_t SignalsImpl::_get = 0;
uint8_t SignalsImpl::_cycles = 0;
SignalsImpl SignalsImpl::_ring[MAX_CYCLES];

uint8_t SignalsImpl::pos() const {
    return this - _ring;
}

const SignalsImpl *SignalsImpl::_prev(uint8_t backward) const {
    return &_ring[(pos() + MAX_CYCLES - backward) % MAX_CYCLES];
}

const SignalsImpl *SignalsImpl::_next(uint8_t forward) const {
    return &_ring[(pos() + forward) % MAX_CYCLES];
}

SignalsImpl *SignalsImpl::_prev(uint8_t backward) {
    return &_ring[(pos() + MAX_CYCLES - backward) % MAX_CYCLES];
}

SignalsImpl *SignalsImpl::_next(uint8_t forward) {
    return &_ring[(pos() + forward) % MAX_CYCLES];
}

uint8_t SignalsImpl::diff(const SignalsImpl *s) const {
    return this < s ? s - this : ((s + MAX_CYCLES) - this) % MAX_CYCLES;
}

void SignalsImpl::discard(const SignalsImpl *s) {
    const auto drop = s->diff(_head());
    if (_cycles < drop) {
        _cycles = 0;
        _put = _get;
    } else {
        _cycles -= drop;
        _put = s->pos();
    }
    _ring[_put].clear();
}

void SignalsImpl::resetCycles() {
    _cycles = 0;
    _ring[_get = _put = 0].clear();
}

void SignalsImpl::nextCycle() {
    _put = (_put + 1) % MAX_CYCLES;
    if (_cycles < MAX_CYCLES) {
        _cycles++;
    } else {
        _get = (_put + 1) % MAX_CYCLES;
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
