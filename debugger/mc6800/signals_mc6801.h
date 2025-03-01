#ifndef __SIGNALS_MC6801_H__
#define __SIGNALS_MC6801_H__

#include "signals_mc6800.h"

namespace debugger {
namespace mc6801 {
struct Signals final : SignalsBase<Signals, mc6800::Signals> {
    void getAddr();
    void getDirection();
    void getData();
    void setData() const;
    static void outputMode();
    static void inputMode();
};
}  // namespace mc6801
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
