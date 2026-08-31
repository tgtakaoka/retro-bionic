#include "signals.h"

namespace debugger {

const SignalsImpl *SignalsImpl::_prev(uint_fast8_t backward) const {
    return Cycles::at(pos() + Cycles::MAX_CYCLES - backward);
}

const SignalsImpl *SignalsImpl::_next(uint_fast8_t forward) const {
    return Cycles::at(pos() + forward);
}

SignalsImpl *SignalsImpl::_prev(uint_fast8_t backward) {
    return Cycles::at(pos() + Cycles::MAX_CYCLES - backward);
}

SignalsImpl *SignalsImpl::_next(uint_fast8_t forward) {
    return Cycles::at(pos() + forward);
}

uint_fast8_t SignalsImpl::diff(const SignalsImpl *s) const {
    return this < s ? s - this
                    : ((s + Cycles::MAX_CYCLES) - this) % Cycles::MAX_CYCLES;
}

}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
