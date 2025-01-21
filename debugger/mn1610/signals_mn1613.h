#ifndef __SIGNALS_MN1613_H__
#define __SIGNALS_MN1613_H__

#include "signals.h"

namespace debugger {
namespace mn1613 {

struct Signals final : SignalsBase<Signals> {
    void getAddress();
    void getControl();
    void getData();
    void getFetch();
    void outData() const;
    static void inputMode();

    bool memory() const;
    bool read() const;
    bool write() const;
    bool fetch() const;

    void print() const;

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace mn1613
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
