#ifndef __SIGNALS_MC68HC05C0_H__
#define __SIGNALS_MC68HC05C0_H__

#include "signals_mc6805.h"

namespace debugger {
namespace mc68hc05c0 {

struct Signals : SignalsBase<Signals, mc6805::Signals> {
    void getControl();
    void getAddr();
    void getData();
    void outData() const;
    bool nobus() const;
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
