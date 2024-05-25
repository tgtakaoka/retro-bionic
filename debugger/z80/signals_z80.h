#ifndef __SIGNALS_Z80_H__
#define __SIGNALS_Z80_H__

#include "signals.h"

namespace debugger {
namespace z80 {

struct Signals final : SignalsBase<Signals> {
    void getAddress();
    void getControl();
    void getData();
    void outData() const;
    void inputMode() const;
    void setAddress(uint16_t _addr) { addr = _addr; }
    void print() const;

    bool nobus() const;
    bool m1() const;
    bool mreq() const;
    bool iorq() const;
    bool read() const;
    bool fetch() const;
    bool intack() const;
    void markRead();

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace z80
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
