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
    void outData() const;
    static void inputMode();
    void print() const;

    bool valid() const;
    bool read() const;
    bool write() const;

    void clearFetch() { _signals[1] = 0; }
    bool fetch() const { return _signals[1] != 0; }
    void markFetch(uint8_t matched) { _signals[1] = matched + 1; }
    uint8_t &fetch() { return _signals[1]; }
    uint8_t matched() const { return _signals[1] - 1; }

protected:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
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
