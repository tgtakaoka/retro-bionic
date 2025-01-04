#ifndef __SIGNALS_PDP8_H__
#define __SIGNALS_PDP8_H__

#include "signals.h"

namespace debugger {
namespace pdp8 {

struct Signals : SignalsBase<Signals> {
    void print() const;

    bool mem() const;
    bool cp() const;
    bool dev() const;
    bool read() const;
    bool fetch() const;

protected:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
    uint8_t dir() const { return _signals[1]; }
    uint8_t &dir() { return _signals[1]; }
    uint8_t ioc() const { return _signals[2]; }
    uint8_t &ioc() { return _signals[2]; }
};

}  // namespace pdp8
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
