#ifndef __SIGNALS_TMS320C15_H__
#define __SIGNALS_TMS320C15_H__

#include "signals.h"

namespace debugger {
namespace tms320c15 {
struct Signals final : SignalsBase<Signals> {
    void getAddr();
    bool getControl();
    void getData();
    void outData() const;
    void inputMode() const;
    void print() const;
    bool fetch() const;  // Program read
    bool write() const;  // Program/IO write
    bool read() const;   // IO read
#ifdef PROFILE_CYCLES
    // Cycles the matcher gave the instruction fetched here, 0 if none.
    uint8_t matched() const { return _signals[1]; }
    void setMatched(uint8_t cycles) { _signals[1] = cycles; }
#endif

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};
}  // namespace tms320c15
}  // namespace debugger
#endif /* __SIGNALS_TMS320C15_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
