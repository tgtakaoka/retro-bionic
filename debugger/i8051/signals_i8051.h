#ifndef __SIGNALS_I8051_H__
#define __SIGNALS_I8051_H__

#include "signals.h"

namespace debugger {
namespace i8051 {

struct Signals final : SignalsBase<Signals> {
    // Inline in pins_i8051.h, for the bus engines.
    inline void getAddress();
    inline void getControl();
    inline void getData();
    inline void setData() const;
    static inline void outputMode();
    static inline void inputMode();
    void print() const;

    inline bool read() const;
    inline bool write() const;
    inline bool fetch() const;
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
