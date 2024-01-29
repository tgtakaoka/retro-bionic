#ifndef __SIGNALS_MC6800_H__
#define __SIGNALS_MC6800_H__

#include "signals.h"

//#define LOG_MATCH(e) e
#define LOG_MATCH(e)

namespace debugger {
namespace mc6800 {
struct Signals : SignalsBase<Signals> {
    void getAddr();
    void getDirection();
    void getData();
    void print() const;

    bool valid() const { return vma() != 0; }
    bool read() const { return rw() != 0; }
    bool write() const { return rw() == 0; }

    void clearFetch() { _signals[2] = 0; }
    void markFetch(uint8_t matched) { _signals[2] = matched + 1; }
    bool fetch() const { return _signals[2] != 0; }
    uint8_t matched() const { return _signals[2] - 1; }

protected:
    uint8_t rw() const { return _signals[0]; }
    uint8_t vma() const { return _signals[1]; }

    uint8_t &rw() { return _signals[0]; }
    uint8_t &vma() { return _signals[1]; }
};
}  // namespace mc6800
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
