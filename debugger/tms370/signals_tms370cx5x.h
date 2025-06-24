#ifndef __SIGNALS_TMS370Cx5x_H__
#define __SIGNALS_TMS370Cx5x_H__

#include "signals_tms370.h"

namespace debugger {
namespace tms370cx5x {

struct SignalsTms370Cx5x final
    : SignalsBase<SignalsTms370Cx5x, tms370::Signals> {
    void getAddr();
    bool getDirection();
    void getData();
    void outData() const;
    void inputMode() const;
};

}  // namespace tms370cx5x
}  // namespace debugger
#endif /* __SIGNALS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
