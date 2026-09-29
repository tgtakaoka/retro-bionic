#ifndef __SIGNALS_I8048_H__
#define __SIGNALS_I8048_H__

#include "signals.h"

namespace debugger {
namespace i8048 {

struct Signals final : SignalsBase<Signals> {
    void getAddress();
    bool getControl();
    void getData();
    void outData() const;
    static void inputMode();
    void print() const;

    bool read() const;
    bool write() const;
    bool fetch() const;
    bool port() const;
    bool valid() const;
    void markInvalid();
    void clearFetch();
#ifdef PROFILE_CYCLES
    // The matcher's verdict: the bus cycles it gave the instruction fetched
    // here, or 0.
    uint8_t matched() const { return _signals[1]; }
    void markFetch(uint8_t matched) { _signals[1] = matched; }
    // Machine cycles with no strobe just before this one.
    uint8_t idles() const { return _signals[2]; }
    uint8_t &idles() { return _signals[2]; }
#endif

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace i8048
}  // namespace debugger
#endif /* __SIGNALS_I8048_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
