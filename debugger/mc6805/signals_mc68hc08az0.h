#ifndef __SIGNALS_MC68HC08AZ0_H__
#define __SIGNALS_MC68HC08AZ0_H__

#include "signals_mc6805.h"

namespace debugger {
namespace mc68hc08az0 {

using mc6805::Signals;

struct SignalsMc68HC08AZ0 final : SignalsBase<SignalsMc68HC08AZ0, Signals> {
    bool getControl();
    void getAddr();
    void getData();
    void outData() const;
    static void inputMode();
};

}  // namespace mc68hc08az0
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
