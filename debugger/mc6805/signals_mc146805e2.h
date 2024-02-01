#ifndef __SIGNALS_MC146805E2_H__
#define __SIGNALS_MC146805E2_H__

#include "signals_mc6805.h"

namespace debugger {
namespace mc146805e2 {

struct Signals : SignalsBase<Signals, mc6805::Signals> {
    void getDirection();
    void getAddr();
    void getLoadInstruction();
    void getData();
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
