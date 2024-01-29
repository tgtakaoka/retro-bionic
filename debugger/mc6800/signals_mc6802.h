#ifndef __SIGNALS_MC6802_H__
#define __SIGNALS_MC6802_H__

#include "signals_mc6800.h"

namespace debugger {
namespace mc6802 {
struct Signals final : SignalsBase<Signals, mc6800::Signals> {
    void getDirection();
    void getAddr();
    void getData();
};
}  // namespace mc6802
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
