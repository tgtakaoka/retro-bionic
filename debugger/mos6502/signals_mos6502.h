#ifndef __SIGNALS_MOS6502_H__
#define __SIGNALS_MOS6502_H__

#include "signals.h"

namespace debugger {
namespace mos6502 {

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getData();
    void print() const;

    bool read() const { return rw() != 0; }
    bool write() const { return rw() == 0; }
    bool fetch() const { return _signals[1]; }
    bool vector() const { return fetchVector(); }

    uint8_t &fetch() { return _signals[1]; }

private:
    uint8_t rw() const { return _signals[0]; }
    uint8_t fetchVector() const { return _signals[2]; }

    uint8_t &rw() { return _signals[0]; }
    uint8_t &fetchVector() { return _signals[2]; }
};

}  // namespace mos6502
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
