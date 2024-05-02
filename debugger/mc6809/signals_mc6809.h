#ifndef __SIGNALS_MC6809_H__
#define __SIGNALS_MC6809_H__

#include "mc6800/signals_mc6800.h"

namespace debugger {
namespace mc6809 {
struct Signals : SignalsBase<Signals, mc6800::Signals> {
    void getHighAddr();
    void getLowAddr();
    void getDirection();
    void clearControl();
    void getData();
    void print() const;

    bool vector() const { return status() == S_VEC; }
    uint8_t lic() const { return _signals[4]; }
    uint8_t avma() const { return _signals[5]; }
    uint8_t &lic() { return _signals[4]; }
    uint8_t &avma() { return _signals[5]; }

protected:
    enum Status : uint8_t {
        S_RUN = 0,   // BA=L, BS=L
        S_VEC = 1,   // BA=L, BS=H
        S_SYNC = 2,  // BA=H, BS=L
        S_HALT = 3,  // BA=H, BS=H
    };

    uint8_t status() const { return _signals[3]; }
    uint8_t &status() { return _signals[3]; }
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
