#ifndef __SIGNALS_MC6809_H__
#define __SIGNALS_MC6809_H__

#include "mc6800/signals_mc6800.h"

namespace debugger {
namespace mc6809 {
struct Signals : SignalsBase<Signals, mc6800::Signals> {
    // Inline in pins_mc6809_base.h, for the bus cycles.
    inline void getHighAddr();
    inline void getLowAddr();
    inline void getDirection();
    inline void getControl();
    inline void getData();
    inline void outData() const;
    static inline void inputMode();
    void print() const;

    inline bool vector() const;
};
}  // namespace mc6809
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
