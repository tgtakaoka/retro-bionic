#ifndef __SIGNALS_TMS9900_H__
#define __SIGNALS_TMS9900_H__

#include "signals_tms9900_base.h"

namespace debugger {
namespace tms9900 {
struct SignalsTms9900 final : SignalsBase<SignalsTms9900, tms9900::Signals> {
    void getAddress();
    void getControl();
    void getData();
    void outData() const;
    void inputMode() const;
};
}  // namespace tms9900
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
