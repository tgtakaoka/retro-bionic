#ifndef __SIGNALS_TLCS90_H__
#define __SIGNALS_TLCS90_H__

#include "signals.h"

#define LOG_MATCH(e)
//#define LOG_MATCH(e) e

namespace debugger {
namespace tlcs90 {

struct Signals final : SignalsBase<Signals> {
    void getAddrHigh();
    void getAddrLow();
    void getDirection();
    void getData();
    void print() const;

    bool read() const;
    bool write() const;

    void clearFetch() { _signals[1] = 0; }
    void markFetch(uint8_t matched) { _signals[1] = matched + 1; }
    bool fetch() const { return _signals[1] != 0; }
    uint8_t matched() const { return _signals[1] - 1; }

    static Signals *current();

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace tlcs90
}  // namespace debugger
#endif /* __SIGNALS_TLCS90_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
