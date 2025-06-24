#ifndef __SIGNALS_TMS370_H__
#define __SIGNALS_TMS370_H__

#include "signals.h"

namespace debugger {
namespace tms370 {
struct Signals : SignalsBase<Signals> {
    bool read() const;
    bool write() const;
    bool fetch() const;

    void print() const;

protected:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};
}  // namespace tms370
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
