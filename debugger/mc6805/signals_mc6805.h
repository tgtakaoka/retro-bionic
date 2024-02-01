#ifndef __SIGNALS_MC6805_H__
#define __SIGNALS_MC6805_H__

#include "signals.h"

//#define LOG(e) e
#define LOG(e)

namespace debugger {
namespace mc6805 {

struct Signals : SignalsBase<Signals> {
    void print() const;

    bool read() const { return rw() != 0; }
    bool write() const { return rw() == 0; }
    bool fetch() const { return li() != 0; }

protected:
    uint8_t rw() const { return _signals[0]; }
    uint8_t li() const { return _signals[1]; }
    uint8_t &rw() { return _signals[0]; }
    uint8_t &li() { return _signals[1]; }
};

}  // namespace mc6805
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
