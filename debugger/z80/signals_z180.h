#ifndef __SIGNALS_Z180_H__
#define __SIGNALS_Z180_H__

#include "signals.h"

namespace debugger {
namespace z180 {

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getAddrHigh();
    bool getControl();
    void getData();
    void outData() const;
    void inputMode() const;
    void setAddress(uint32_t _addr) { addr = _addr; }
    void print() const;

    bool fetch() const;
    bool mreq() const;
    bool iorq() const;
    bool nobus() const;
    bool read() const;
    bool write() const;
    bool intack() const;

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace z180
}  // namespace debugger
#endif /* __SIGNALS_Z180_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
