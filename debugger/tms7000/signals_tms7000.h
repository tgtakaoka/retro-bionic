#ifndef __SIGNALS_TMS7000_H__
#define __SIGNALS_TMS7000_H__

#include "signals.h"

namespace debugger {
namespace tms7000 {

struct Signals final : SignalsBase<Signals> {
    // Inline in pins_tms7000.h, for the bus engines.
    inline void getAddress();
    inline void getDirection();
    inline void getData();
    inline void outData() const;
    inline void inputMode() const;
    void print() const;

    inline bool external() const;
    inline bool read() const;
    inline bool write() const;
    bool fetch() const { return _signals[1]; }
    void markFetch(bool fetch) { _signals[1] = fetch; }
#ifdef PROFILE_CYCLES
    uint8_t matched() const { return _signals[2]; }
    void setMatched(uint8_t cycles) { _signals[2] = cycles; }
#endif
    inline bool intack() const;

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace tms7000
}  // namespace debugger
#endif /* __SIGNALS_TMS7000_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
