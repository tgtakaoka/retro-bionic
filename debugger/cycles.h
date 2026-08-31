#ifndef __DEBUGGER_CYCLES_H__
#define __DEBUGGER_CYCLES_H__

#include <stdint.h>

namespace debugger {

struct SignalsImpl;

/**
 * The ring buffer of captured bus cycles.
 *
 * Storage is deliberately untyped so that a single ring serves every target;
 * |SignalsBase| casts the entries to the concrete per-architecture type.
 */
struct Cycles {
    static constexpr uint_fast8_t MAX_CYCLES = 128;

    static void reset();
    static void next();
    static void discard(const SignalsImpl *s);

    static uint_fast8_t cycles() { return _cycles; }

    static SignalsImpl *head();
    static SignalsImpl *tail();
    static SignalsImpl *at(uint_fast8_t index);
    static uint_fast8_t indexOf(const SignalsImpl *s);

private:
    static uint_fast8_t _put;
    static uint_fast8_t _get;
    static uint_fast8_t _cycles;
    static SignalsImpl _ring[MAX_CYCLES];
};

}  // namespace debugger
#endif /* __CYCLES_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
