#ifndef __SIGNALS_KL5C8400_H__
#define __SIGNALS_KL5C8400_H__

#include "signals.h"

namespace debugger {
namespace kl5c8400 {

struct Signals final : SignalsBase<Signals> {
    void getAddress();
    bool getControl();
    // A strobe on the bus, read without recording it.
    static bool busy();
    void noCycle();
    void getData();
    void outData() const;
    void inputMode() const;
    void setAddress(uint16_t _addr) { addr = _addr; }
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

}  // namespace kl5c8400
}  // namespace debugger
#endif /* __SIGNALS_KL5C8400_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
