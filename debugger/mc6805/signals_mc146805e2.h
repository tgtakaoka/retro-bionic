#ifndef __SIGNALS_MC146805E2_H__
#define __SIGNALS_MC146805E2_H__

#include "signals_mc6805.h"

namespace debugger {
namespace mc146805e2 {

    using mc6805::Signals;

struct SignalsMc146805E2 : SignalsBase<SignalsMc146805E2, Signals> {
    void getControl();
    void getAddr();
    void getData();
    void outData() const;
    static void inputMode();
};

}  // namespace mc146805e2
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
