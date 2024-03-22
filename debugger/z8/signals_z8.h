#ifndef __SIGNALS_Z8_H__
#define __SIGNALS_Z8_H__

#include "signals.h"

namespace debugger {
namespace z8 {

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getData();
    void outData();
    void print() const;

    bool read() const { return rw() != 0; }
    bool write() const { return rw() == 0; }
    bool fetch() const { return _signals[1]; }
    uint8_t &fetch() { return _signals[1]; }

private:
    uint8_t rw() const { return _signals[0]; }
    uint8_t &rw() { return _signals[0]; }
};

}  // namespace z8
}  // namespace debugger
#endif /* __SIGNALS_Z86_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
