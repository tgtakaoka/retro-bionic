#ifndef __SIGNALS_MC68HC05C0_H__
#define __SIGNALS_MC68HC05C0_H__

#include "signals_mc6805.h"

namespace debugger {
namespace mc68hc05c0 {

using mc6805::Signals;

struct SignalsMc68HC05C0 : SignalsBase<SignalsMc68HC05C0, Signals> {
    // Inline in the pins header, for the bus cycles.
    inline void getControl();
    inline void getAddr();
    inline void getData();
    inline void outData() const;
    static inline void inputMode();
};

}  // namespace mc68hc05c0
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
