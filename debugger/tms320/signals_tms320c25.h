#ifndef __SIGNALS_TMS320C25_H__
#define __SIGNALS_TMS320C25_H__

#include "signals.h"

namespace debugger {
namespace tms320c25 {
struct Signals final : SignalsBase<Signals> {
    void getAddr();
    /** Whether #STRB frames a bus cycle. */
    bool getControl();
    void getData();
    void outData() const;
    void inputMode() const;
    void print() const;

    bool strobe() const;  // #STRB framed this cycle
    bool read() const;
    bool ps() const;  // program space
    bool ds() const;  // data space
    bool is() const;  // I/O space
    bool programRead() const { return strobe() && ps() && read(); }

private:
    uint8_t cntl() const { return _signals[0]; }
    uint8_t &cntl() { return _signals[0]; }
};
}  // namespace tms320c25
}  // namespace debugger
#endif /* __SIGNALS_TMS320C25_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
