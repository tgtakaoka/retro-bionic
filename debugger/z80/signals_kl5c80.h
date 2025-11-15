#ifndef __SIGNALS_KL5C80_H__
#define __SIGNALS_KL5C80_H__

#include "signals.h"

namespace debugger {
namespace kl5c80 {

struct Signals final : SignalsBase<Signals> {
    void getAddr();
    void getAddrHigh();
    bool getControl();
    void getData();
    void outData() const;
    void inputMode() const;
    void setAddress(uint32_t _addr) { addr = _addr; }
    void print() const;

    bool nobus() const;
    bool fetch() const;
    bool mrd() const;
    bool mwr() const;
    bool iord() const;
    bool iowr() const;

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};

}  // namespace kl5c80
}  // namespace debugger
#endif /* __SIGNALS_KL5C80_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
