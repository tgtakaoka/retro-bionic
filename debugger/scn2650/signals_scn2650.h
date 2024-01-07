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

    bool read() const { return rw() == 0; }
    bool write() const { return rw() != 0; }
    bool io() const { return mio() == 0; }
    bool vector() const { return intack() != 0; }
    bool fetch() const { return _signals[3]; }
    uint8_t &fetchMark() { return _signals[3]; }

private:
    uint8_t rw() const { return _signals[0]; }
    uint8_t mio() const { return _signals[1]; }
    uint8_t intack() const { return _signals[2]; }

    uint8_t &rw() { return _signals[0]; }
    uint8_t &mio() { return _signals[1]; }
    uint8_t &intack() { return _signals[2]; }
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
