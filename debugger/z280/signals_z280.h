#ifndef __SIGNALS_Z280_H__
#define __SIGNALS_Z280_H__

#include "signals.h"

namespace debugger {
namespace z280 {
struct Signals final : SignalsBase<Signals> {
    void getAddr();
    bool getControl();
    void getData();
    void outData() const;
    void inputMode() const;
    void print() const;
    bool fetch() const;  // Program read
    bool write() const;  // Program/IO write
    bool read() const;   // IO read

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};
}  // namespace z280
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
