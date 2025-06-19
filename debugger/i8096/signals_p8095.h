#ifndef __SIGNALS_P8095_H__
#define __SIGNALS_P8095_H__

#include "signals.h"

namespace debugger {
namespace p8095 {
struct Signals final : SignalsBase<Signals> {
    bool addrValid() const;
    void getAddr();
    bool getControl();
    void getData();
    void outData() const;
    void inputMode() const;
    void print() const;
    bool fetch() const;  // Instruction fetch
    bool read() const;   // Read
    bool write() const;  // Write

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};
}  // namespace p8095
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
