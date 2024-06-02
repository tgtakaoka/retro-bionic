#ifndef __SIGNALS_TMS7000_H__
#define __SIGNALS_TMS7000_H__

#include "signals.h"

namespace debugger {
namespace tms7000 {

struct Signals final : SignalsBase<Signals> {
    void getAddress();
    void getDirection();
    void getData();
    void outData();
    void inputMode() const;
    void print() const;

    bool external() const;
    bool read() const;
    bool write() const;
    bool fetch() const { return _signals[1]; }
    void markFetch(bool fetch) { _signals[1] = fetch; }
    bool intack() const;

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace tms7000
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
