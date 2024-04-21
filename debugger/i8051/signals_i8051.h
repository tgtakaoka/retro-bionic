#ifndef __SIGNALS_I8051_H__
#define __SIGNALS_I8051_H__

#include "signals.h"

namespace debugger {
namespace i8051 {

struct Signals final : SignalsBase<Signals> {
    void getAddress();
    bool getControl();
    void getData();
    void print() const;

    bool read() const;
    bool write() const;
    bool fetch() const;
    void clearFetch();

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace i8051
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
