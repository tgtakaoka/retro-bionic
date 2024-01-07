#ifndef __SIGNALS_Z88_H__
#define __SIGNALS_Z88_H__

#include "signals.h"

namespace debugger {
namespace z88 {

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getDirection();
    void getData();
    void outData();
    void print() const;

    bool read() const { return rw() != 0; }
    bool write() const { return rw() == 0; }
    bool fetch() const { return _signals[2]; }
    uint8_t &fetch() { return _signals[2]; }

private:
    uint8_t rw() const { return _signals[0]; }
    uint8_t dm() const { return _signals[1]; }
    uint8_t &rw() { return _signals[0]; }
    uint8_t &dm() { return _signals[1]; }
};

}  // namespace z88
}  // namespace debugger
#endif /* __SIGNALS_Z88_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
