#ifndef __SIGNALS_SCN2650_H__
#define __SIGNALS_SCN2650_H__

#include "signals.h"

namespace debugger {
namespace scn2650 {

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getData();
    void outData();
    void print() const;

    bool read() const;
    bool write() const;
    bool io() const;
    bool vector() const;
    bool fetch() const { return (cntl() & CNTL_FETCH) != 0; }
    void markFetch() { cntl() |= CNTL_FETCH; }
    void clearFetch() { cntl() &= ~CNTL_FETCH; }

private:
    static constexpr uint8_t CNTL_FETCH = 0x01;

    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace scn2650
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
