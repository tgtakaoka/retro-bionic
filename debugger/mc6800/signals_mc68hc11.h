#ifndef __SIGNALS_MC68HC11_H__
#define __SIGNALS_MC68HC11_H__

#include "signals_mc6800.h"

namespace debugger {
namespace mc68hc11 {

struct Signals final : SignalsBase<Signals, mc6800::Signals> {
    void getAddr();
    void getDirection();
    void getControl();
    void getData();
    void outData() const;
    static void inputMode();
};

}  // namespace mc68hc11
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
