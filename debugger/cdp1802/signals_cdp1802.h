#ifndef __SIGNALS_CDP1802_H__
#define __SIGNALS_CDP1802_H__

#include "signals.h"

namespace debugger {
namespace cdp1802 {

struct Signals final : SignalsBase<Signals> {
    void getStatus();
    void getAddr1();
    void getAddr2();
    void getDirection();
    void getData();
    void print() const;

    bool read() const { return mrd() == 0; }
    bool write() const { return mwr() == 0; }
    bool fetch() const { return sc() == 0; }
    bool vector() const { return sc() == 3; }

private:
    uint8_t sc() const { return _signals[0]; }
    uint8_t mrd() const { return _signals[1]; }
    uint8_t mwr() const { return _signals[2]; }

    uint8_t &sc() { return _signals[0]; }
    uint8_t &mrd() { return _signals[1]; }
    uint8_t &mwr() { return _signals[2]; }
};

}  // namespace cdp1802
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
