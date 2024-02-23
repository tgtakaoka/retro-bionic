#ifndef __SIGNALS_MC6809E_H__
#define __SIGNALS_MC6809E_H__

#include "signals_mc6809.h"

namespace debugger {
namespace mc6809e {
struct Signals : SignalsBase<Signals, mc6809::Signals> {
    void getControl();
};
}  // namespace mc6809e
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
