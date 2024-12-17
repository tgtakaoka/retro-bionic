#ifndef __SIGNALS_MC6809_H__
#define __SIGNALS_MC6809_H__

#include "mc6800/signals_mc6800.h"

namespace debugger {
namespace mc6809 {
struct Signals : SignalsBase<Signals, mc6800::Signals> {
    void getHighAddr();
    void getLowAddr();
    void getDirection();
    void getControl();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const;

    bool vector() const;
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
