#ifndef __SIGNALS_TMS9995_H__
#define __SIGNALS_TMS9995_H__

#include "signals_tms9900.h"

namespace debugger {
namespace tms9995 {
struct Signals final : SignalsBase<Signals, tms9900::Signals> {
    void getAddress();
    void getControl();
    void getData();
    void outData() const;
    void inputMode() const;
};
}  // namespace tms9995
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4: