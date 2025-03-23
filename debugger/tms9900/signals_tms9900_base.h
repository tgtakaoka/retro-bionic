#ifndef __SIGNALS_TMS9900_BASE_H__
#define __SIGNALS_TMS9900_BASE_H__

#include "signals.h"

namespace debugger {
namespace tms9900 {
struct Signals : SignalsBase<Signals> {
    bool memory() const;
    bool read() const;
    bool write() const;
    bool fetch() const;
    void clearFetch();
    bool data16() const;
    bool hasBst() const;

    void print() const;

protected:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
    uint8_t bst() const { return _signals[1]; }
    uint8_t &bst() { return _signals[1]; }
};
}  // namespace tms9900
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
