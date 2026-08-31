#ifndef __DEBUGGER_SIGNALS_H__
#define __DEBUGGER_SIGNALS_H__

#include <stdint.h>

#include "cycles.h"

namespace debugger {

struct SignalsImpl {
    uint32_t addr;
    uint16_t data;

    void clear() { _flags = 0; }
    bool readMemory() const { return (_flags & INJECT) == 0; }
    bool writeMemory() const { return (_flags & CAPTURE) == 0; }

    uint_fast8_t pos() const { return Cycles::indexOf(this); }
    uint_fast8_t diff(const SignalsImpl *s) const;

protected:
    uint8_t _signals[5];
    uint8_t _flags;

    void _inject(uint16_t val) {
        data = val;
        _flags |= INJECT;
    }
    void _capture() { _flags |= CAPTURE; }
    void _clearInject() { _flags &= ~INJECT; }
    const SignalsImpl *_prev(uint_fast8_t backward) const;
    const SignalsImpl *_next(uint_fast8_t forward) const;
    SignalsImpl *_prev(uint_fast8_t backward);
    SignalsImpl *_next(uint_fast8_t forward);

private:
    // _flags
    static constexpr uint8_t INJECT = 1;
    static constexpr uint8_t CAPTURE = 2;
};

template <typename SIGNALS_T, typename IMPL_T = SignalsImpl>
struct SignalsBase : IMPL_T {
    SIGNALS_T *inject(uint16_t data) {
        IMPL_T::_inject(data);
        return static_cast<SIGNALS_T *>(this);
    }

    SIGNALS_T *clearInject() {
        IMPL_T::_clearInject();
        return static_cast<SIGNALS_T *>(this);
    }

    SIGNALS_T *capture() {
        IMPL_T::_capture();
        return static_cast<SIGNALS_T *>(this);
    }

    const SIGNALS_T *prev(uint_fast8_t backward = 1) const {
        return static_cast<const SIGNALS_T *>(IMPL_T::_prev(backward));
    }

    const SIGNALS_T *next(uint_fast8_t forward = 1) const {
        return static_cast<const SIGNALS_T *>(IMPL_T::_next(forward));
    }

    SIGNALS_T *prev(uint_fast8_t backward = 1) {
        return static_cast<SIGNALS_T *>(IMPL_T::_prev(backward));
    }

    SIGNALS_T *next(uint_fast8_t forward = 1) {
        return static_cast<SIGNALS_T *>(IMPL_T::_next(forward));
    }

    static SIGNALS_T *put() { return static_cast<SIGNALS_T *>(Cycles::head()); }

    static SIGNALS_T *get() { return static_cast<SIGNALS_T *>(Cycles::tail()); }
};

}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
