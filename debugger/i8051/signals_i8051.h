#ifndef __SIGNALS_I8051_H__
#define __SIGNALS_I8051_H__

#include "signals.h"

namespace debugger {
namespace i8051 {

struct Signals final : SignalsBase<Signals> {
    void getAddress();
    void getControl();
    void getData();
    void setData() const;
    static void outputMode();
    static void inputMode();
    void print() const;

    bool read() const;
    bool write() const;
    bool fetch() const;
    // A bus cycle that didn't happen: taken as a read, not a write.
    void noCycle();
    void clearFetch();
#ifdef PROFILE_CYCLES
    // Bus cycles the matcher gave the instruction fetched here, 0 if none.
    uint8_t matched() const { return _signals[1]; }
    void setMatched(uint8_t cycles) { _signals[1] = cycles; }
#endif

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace i8051
}  // namespace debugger
#endif /* __SIGNALS_I8051_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
