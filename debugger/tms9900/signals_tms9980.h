#ifndef __SIGNALS_TMS9980_H__
#define __SIGNALS_TMS9980_H__

#include "signals_tms9900_base.h"

namespace debugger {
namespace tms9980 {
struct SignalsTms9980 final : SignalsBase<SignalsTms9980, tms9900::Signals> {
    void getLowAddr();
    void getHighAddr();
    void getControl();
    void getData();
    void outData() const;
    void inputMode() const;
};
}  // namespace tms9980
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
